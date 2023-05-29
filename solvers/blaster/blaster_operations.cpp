/* blaster_operations.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"
#include "solvers/blaster/blaster_bits.hpp"

using namespace ismt;


// ------------math operations------------
// math operations
bvar blaster_solver::Negate(bvar var){
    if(var->isConstant()){
        return mkInt(-var->getCurValue());
    }
    bvar t = Invert(var);
    bvar ans = AddOne(t);
    ans->setName("(-"+var->getName()+")");
    return ans;
}
bvar blaster_solver::Absolute(bvar var){
    if(var->isConstant()){
        if(var->signBit()==1) return Negate(var);
        else return var;
    }
    bvar neg = Negate(var);
    // var->SignBit() == 1?(-var):var. 
    return Ite_num(var->signBit(), neg, var);
}
bvar blaster_solver::Subtract(bvar var1, bvar var2){
    return Add(var1, Negate(var2));
}
bvar blaster_solver::Ite_num (literal cond, bvar ifp, bvar elp){
    if(cond==solver->True()) return ifp;
    else if(cond==solver->False()) return elp;
    
    bvar tmp = mkInnerVar("ite("+to_string(cond)+","+ifp->getName()+","+elp->getName()+")", 
                    max(ifp->csize(), elp->csize()));
    
    // debug: 2021-11-08, it can be integer.
    bool_var teqi;bool_var teqe;
    if(ifp->isConstant()) teqi = viEq(tmp, ifp);
    else teqi = vvEq(tmp, ifp);
    if(elp->isConstant()) teqe = viEq(tmp, elp);
    else teqe = vvEq(tmp, elp);

    // cond -> teqi
    clause c;
    c.emplace_back(-cond);
    c.emplace_back(teqi);
    addClause(c);

    // ~cond -> teqe
    c.clear();
    c.emplace_back(cond);
    c.emplace_back(teqe);
    addClause(c);

    // teqi \/ teqe
    c.clear();
    c.emplace_back(teqi);
    c.emplace_back(teqe);
    addClause(c);

    return tmp;
}

// definition of the functions div and mod:
// Boute, Raymond T. (April 1992). 
// The Euclidean definition of the functions div and mod. 
// ACM Transactions on Programming Languages and Systems (TOPLAS) 
// ACM Press. 14 (2): 127 - 144. doi:10.1145/128861.128862.
bvar blaster_solver::Divide(bvar var1, bvar var2){
    // -3 / 2 = -2 ... 1
    // -2 / 3 = -1 ... 1
    // 3 / -2 = -1 ... 1
    // 2 / -3 =  0 ... 2
    // -3 / -2 = 2 ... 1
    // -2 / -3 = 1 ... 1
    if(var1->isConstant()&&var2->isConstant()){
        return mkInt(var1->getCurValue()/var2->getCurValue());
    }
    // some cases can simplify.
    if(var1->isConstant()){
        if(var1->getCurValue()==0) return mkInt(0);
    }
    else if(var2->isConstant()){
        if(var2->getCurValue()==0) outputError("Divided by 0!");
        else if(var2->getCurValue()==1) return var1;
        else if(var2->getCurValue()==-1) return Negate(var1);
        else if(divideZero(var2->getCurValue()) >= var1->csize()) return mkInt(0); // 2 / 4 = 0. 
        else if(var2->getCurValue()%2==0){
            Integer num = var2->getCurValue();
            unsigned len = compress(num);
            
            // debug: 2021.11.27
            // -1(11111) >> 1 = (-1)1111, it will never change.
            bvar stmp = Shift(var1, len);
            bvar tmp = nullptr;
            if(num==1) tmp = stmp;
            else if(num==-1) tmp = Negate(stmp);
            else tmp = Divide(stmp, mkInt(num));
            if(var2->getCurValue()<0){
                // 2 / -3 =  0 ... 2
                return Ite_num(And(viGt(Absolute(var2), Absolute(var1)), GeZero(var1)), mkInt(0), tmp);
            }
            else{
                // -3 / 2 = -2 ... 1
                // -2 / 3 = -1 ... 1
                // 3 / -2 = -1 ... 1
                // -3 / -2 = 2 ... 1
                // -2 / -3 = 1 ... 1
                return tmp;
            }
        }
        // if var1 = x - 32(from left bound), var1 >> 3 ? do like z3?
    }
    bvar abv2 = Absolute(var2);
    // TODO, can simplify?
    bvar q = Ite_num(vvGt(abv2, Absolute(var1)), mkInt(0), mkInnerVar("("+var1->getName()+"/"+var2->getName()+")", var1->csize()));
    bvar r = mkInnerVar("("+var1->getName()+"%"+var2->getName()+")", var2->csize());
    
    // must use /, once letvar use /, we may not multiply to integer.
    // special cases
    if(var1->isConstant() || var2->isConstant()){
        innerEqualBit((var1->isConstant()?viEq(Negate(var1), var2):viEq(var1, Negate(var2))), And(EqualInt(q, mkInt(-1)), EqualInt(r, mkInt(0))));
        innerEqualBit(viEq(var1, var2), And(EqualInt(q, mkInt(1)), EqualInt(r, mkInt(0))));
    }
    else{
        innerEqualBit(Or(vvEq(Negate(var1), var2), vvEq(var1, Negate(var2))), And(EqualInt(q, mkInt(-1)), EqualInt(r, mkInt(0))));
        innerEqualBit(vvEq(var1, var2), And(EqualInt(q, mkInt(1)), EqualInt(r, mkInt(0))));
    }
    innerEqualVar(Add(Multiply(var2, q), r), var1);
    if(!var2->isConstant()) innerEqualBitInt(NeqZero(var2), 1); // var2 != 0
    innerEqualBitInt(GeZero(r), 1); // r >= 0
    // TODO, -3 / 2 = -2 ... 1
    // Greater(r, Negate(abv2)); // r > -var2
    innerEqualBitInt(Less(r, abv2), 1); // r < var2 
    return q;
}
bvar blaster_solver::Modulo(bvar var1, bvar var2){
    if(var1->isConstant()&&var2->isConstant()){
        return mkInt(var1->getCurValue()%var2->getCurValue());
    }
    // some cases can simplify.
    if(var1->isConstant()){
        if(var1->getCurValue()==0) return mkInt(0);
    }
    else if(var2->isConstant()){
        if(var2->getCurValue()==0) outputError("Modulo by 0!");
        else if(var2->getCurValue()==1) return mkInt(0);
        else if(divideZero(var2->getCurValue()) >= var1->csize()) return var1; // 2 % 4 = 2.
    }

    bvar q = Ite_num(vvGt(Absolute(var2),Absolute(var1)), mkInt(0), mkInnerVar("("+var1->getName()+"/"+var2->getName()+")", var1->size() - var2->size() + 1));
    bvar r = mkInnerVar("("+var1->getName()+"%"+var2->getName()+")", var2->size());
    innerEqualVar(Add(Multiply(var2, q), r), var1);
    if(!var2->isConstant()) innerEqualBitInt(NeqZero(var2), 1); // var2 != 0
    innerEqualBitInt(GeZero(r), 1); // r >= 0
    innerEqualBitInt(Less(r, Absolute(var2)), 1); // r < var2 
    return r;

}
// main functions, add and multiply.
bvar blaster_solver::Add(bvar var1, bvar var2){
    if(var1->isConstant() && var2->isConstant()) 
        return mkInt(var1->getCurValue() + var2->getCurValue());
    assert(!var1->isConstant() || !var2->isConstant());
    // some cases that can simplify
    // debug 2021-11-06: add one may overflow ( not AddOne ), excpet it is inverted.
    if(var1->isConstant() && var1->getCurValue()==0) return var2;
    else if(var2->isConstant() && var2->getCurValue()==0) return var1;
    return Add(var1, var2, false);
}
bvar blaster_solver::Multiply(bvar var1, bvar var2){
    if(var1->isConstant() && var2->isConstant()) 
        return mkInt(var1->getCurValue() * var2->getCurValue());
    assert(!var1->isConstant() || !var2->isConstant());
    // some cases that can simplify
    if(var1->isConstant()){
        if(var1->getCurValue()==0) return mkInt(0);
        else if(var1->getCurValue()==1) return var2;
        else if(var1->getCurValue()==-1) return Negate(var2);
        else if(var1->getCurValue()%2==0){
            Integer num = var1->getCurValue();
            unsigned len = compress(num);
            bvar stmp = Shift(var2, -(int)len);
            if(num==1) return stmp;
            else return Multiply(stmp, mkInt(num));
        }
        // mk it positive.
        if(var1->getCurValue()<0){
            var2 = Negate(var2);
            var1 = mkInt(-var1->getCurValue());
        }
        assert(var1->getCurValue()>0);
        return MultiplyInt(var2, var1);
    }
    else if(var2->isConstant()){
        if(var2->getCurValue()==0) return mkInt(0);
        else if(var2->getCurValue()==1) return var1;
        else if(var2->getCurValue()==-1) return Negate(var1);
        else if(var2->getCurValue()%2==0){
            Integer num = var2->getCurValue();
            unsigned len = compress(num);
            bvar stmp = Shift(var1, -(int)len);
            if(num==1) return stmp;
            else return Multiply(stmp, mkInt(num));
        }
        // mk it positive.
        if(var2->getCurValue()<0){
            var1 = Negate(var1);
            var2 = mkInt(-var2->getCurValue());
        }
        assert(var2->getCurValue()>0);
        return MultiplyInt(var1, var2);
    }
    // they are both variables.
    bvar varmax;bvar varmin;
    if(var1->size()>=var2->size()){varmax = var1; varmin = var2;}
    else{varmax = var2; varmin = var1;}
    unsigned len = var1->csize() + var2->csize();
    bvar answer = mkInnerVar("("+var1->getName()+"*"+ var2->getName()+")", len);
    if(var1==var2){
        // square
        innerEqualBitInt(answer->signBit(), 0); // square, then must positive.
    }
    else if(!varmax->hasZero() && !varmin->hasZero()){// both variables cannot be zero
        doTargetInt(answer->signBit(), varmax->signBit(), 0, varmin->signBit()); // sign bit
    }
    else{
        bool_var tt = solver->False();
        if(varmax->hasZero() && varmin->hasZero()){
            tt = Or(EqZero(varmax), EqZero(varmin));
        }
        else if(varmax->hasZero()){
            tt = EqZero(varmax);
        }
        else if(varmin->hasZero()){
            tt = EqZero(varmin);
        }
        innerEqualBit(tt, EqZero(answer));
        bool_var t = newSatVar();
        doTargetInt(t, varmax->signBit(), 0, varmin->signBit()); // sign bit: t = xor(varmax->signBit(), varmin->signBit());
        innerEqualBit(answer->signBit(), Ite_bool(tt, solver->False(), t));
    }
    bvar subsum = MultiplyBit(varmax, varmin->getAt(1), len); // varmax * varmin[1]
    for(size_t i=2;i<varmin->size();i++){
        bvar subans = MultiplyBit(varmax, varmin->getAt(i), len+1-i);
        subsum = ShiftAdd(subsum, subans, answer, i);
    }
    for(size_t i=varmin->size();i<answer->size();i++){
        // extend the varmin's signbit.
        bvar subans = MultiplyBit(varmax, varmin->signBit(), len+1-i);
        subsum = ShiftAdd(subsum, subans, answer, i);
    }
    innerEqualBit(subsum->getAt(1), answer->getAt(len));
    return answer;
}

// auxiliary functions for math operations
bvar blaster_solver::Add(bvar var1, bvar var2, bool addone){
    bvar varmax;bvar varmin;
    if(var1->size()>=var2->size()) varmax = var1, varmin = var2;
    else varmax = var2, varmin = var1;
    unsigned size = 0;
    if(addone) size = varmax->csize();
    else size = varmax->size();
    bvar cry = mkInnerVar("auxVar_addcry("+ to_string(auxSize++) +")", size+1);
    bvar answer = mkInnerVar("auxVar_addans("+ to_string(auxSize++) +")", size);
    innerEqualBitInt(cry->signBit(), 0);
    innerEqualBitInt(cry->getAt(1), 0);
    // if one is constant
    if(var1->isConstant() || var2->isConstant()){
        bvar var;bvar v;
        if(var1->isConstant()) var = var2, v = var1;
        else var = var1, v = var2;

        // lower bits of var + v
        for(size_t i=1;i<varmin->size();i++){ // answer[1,...,lenmin-1]
            doTargetInt(answer->getAt(i), var->getAt(i), v->getAt(i), cry->getAt(i));
        }
        for(size_t i=1;i<varmin->size();i++){ // carry[2,...,lenmin]
            doCarryInt(cry->getAt(i+1), var->getAt(i), v->getAt(i), cry->getAt(i));
        }
        if(var->size()>=v->size()){
            // upper bits adding: extend v with its sign bit
            for(size_t i=varmin->size();i<varmax->size();i++){ // answer[lenmin, ..., lenmax]
                doTargetInt(answer->getAt(i), var->getAt(i), v->signBit(), cry->getAt(i));
            }
            for(size_t i=varmin->size();i<varmax->size();i++){ // carry[lenmin+1, ..., lenmax+1]
                doCarryInt(cry->getAt(i+1), var->getAt(i), v->signBit(), cry->getAt(i));
            }
            // upper bits adding: extend var, v with its sign bit
            for(size_t i=varmax->size();i<answer->size();i++){ // answer[lenmin, ..., size]
                doTargetInt(answer->getAt(i), var->signBit(), v->signBit(), cry->getAt(i));
            }
            for(size_t i=varmax->size();i<answer->size();i++){ // carry[lenmin+1, ..., size+1]
                doCarryInt(cry->getAt(i+1), var->signBit(), v->signBit(), cry->getAt(i));
            }
        }
        else{ // var->size()<v->size()
            // upper bits adding: extend var with its sign bit
            for(size_t i=varmin->size();i<varmax->size();i++){ // answer[lenmin, ..., lenmax]
                doTargetInt(answer->getAt(i), var->signBit(), v->getAt(i), cry->getAt(i));
            }
            for(size_t i=varmin->size();i<varmax->size();i++){ // carry[lenmin+1, ..., lenmax+1]
                doCarryInt(cry->getAt(i+1), var->signBit(), v->getAt(i), cry->getAt(i));
            }
            // upper bits adding: extend var, v with its sign bit
            for(size_t i=varmax->size();i<answer->size();i++){ // answer[lenmin, ..., size]
                doTargetInt(answer->getAt(i), var->signBit(), v->signBit(), cry->getAt(i));
            }
            for(size_t i=varmax->size();i<answer->size();i++){ // carry[lenmin+1, ..., size+1]
                doCarryInt(cry->getAt(i+1), var->signBit(), v->signBit(), cry->getAt(i));
            }
        }
        // calculate sign bit of answer
        // 2021.11.13, if use as add one, the sign bit will not change.
        // if(addone) innerEqualBit(answer->signBit(), var->signBit());
        // else doTargetInt(answer->signBit(), var->signBit(), v->signBit(), cry->getAt(size+1));
        doTargetInt(answer->signBit(), var->signBit(), v->signBit(), cry->getAt(size+1));
    }
    else{ // they are both variables.
        // lower bits of varmax + varmin    
        for(size_t i=1;i<varmin->size();i++){ // answer[1,...,lenmin-1]
            doTarget(answer->getAt(i), varmax->getAt(i), varmin->getAt(i), cry->getAt(i));
        }
        for(size_t i=1;i<varmin->size();i++){ // carry[2,...,lenmin]
            doCarry(cry->getAt(i+1), varmax->getAt(i), varmin->getAt(i), cry->getAt(i));
        }

        // upper bits adding: extend varmin with its sign bit
        for(size_t i=varmin->size();i<varmax->size();i++){ // answer[lenmin, ..., lenmax]
            doTarget(answer->getAt(i), varmax->getAt(i), varmin->signBit(), cry->getAt(i));
        }
        for(size_t i=varmin->size();i<varmax->size();i++){ // carry[lenmin+1, ..., lenmax+1]
            doCarry(cry->getAt(i+1), varmax->getAt(i), varmin->signBit(), cry->getAt(i));
        }

        // upper bits adding: extend varmax, varmin with its sign bit
        for(size_t i=varmax->size();i<answer->size();i++){ // answer[lenmin, ..., lenmax]
            doTarget(answer->getAt(i), varmax->signBit(), varmin->signBit(), cry->getAt(i));
        }
        for(size_t i=varmax->size();i<answer->size();i++){ // carry[lenmin+1, ..., lenmax+1]
            doCarry(cry->getAt(i+1), varmax->signBit(), varmin->signBit(), cry->getAt(i));
        }

        // calculate sign bit of answer
        doTarget(answer->signBit(), varmax->signBit(), varmin->signBit(), cry->getAt(size+1));
    }

    return answer;
}
bvar blaster_solver::MultiplyInt(bvar var, bvar v){
    bvar subsum = mkInt(0);
    for(size_t i=1;i<v->size();i++){
        if(v->getAt(i)==1) subsum = Add(subsum, Shift(var, -(int)(i-1)));
    }
    return subsum;
}
bvar blaster_solver::MultiplyBit(bvar var, literal bit, unsigned len){
    bvar answer = mkInnerVar("auxVar_submuli("+to_string(auxSize++)+")", len);
    if(answer->size()<=var->size()){
        for(size_t i=1;i<answer->size();i++){
            // answer[i] == var[i] /\ bit

            // ~answer[i] /\ var[i]
            clause c;
            c.emplace_back(-answer->getAt(i));
            c.emplace_back(var->getAt(i));
            addClause(c);
            // ~answer[i] /\ bit
            c.clear();
            c.emplace_back(-answer->getAt(i));
            c.emplace_back(bit);
            addClause(c);
            // answer[i] \/ ~bit \/ ~var[i]
            c.clear();
            c.emplace_back(answer->getAt(i));
            c.emplace_back(-bit);
            c.emplace_back(-var->getAt(i));
            addClause(c);

        }
    }
    else{ // answer->size() > var->size()
        for(size_t i=1;i<var->size();i++){
            // answer[i] == var[i] /\ bit
            // ~answer[i] \/ var[i]
            clause c;
            c.emplace_back(-answer->getAt(i));
            c.emplace_back(var->getAt(i));
            addClause(c);
            // ~answer[i] \/ bit
            c.clear();
            c.emplace_back(-answer->getAt(i));
            c.emplace_back(bit);
            addClause(c);
            // answer[i] \/ ~bit \/ ~var[i]
            c.clear();
            c.emplace_back(answer->getAt(i));
            c.emplace_back(-bit);
            c.emplace_back(-var->getAt(i));
            addClause(c);

        }
        // extend var's sign bit.
        for(size_t i=var->size();i<answer->size();i++){
            // answer[i] == var[0] /\ bit

            // ~answer[i] \/ var[i]
            clause c;
            c.emplace_back(-answer->getAt(i));
            c.emplace_back(var->signBit());
            addClause(c);
            // ~answer[i] \/ bit
            c.clear();
            c.emplace_back(-answer->getAt(i));
            c.emplace_back(bit);
            addClause(c);
            // answer[i] \/ ~bit \/ ~var[i]
            c.clear();
            c.emplace_back(answer->getAt(i));
            c.emplace_back(-bit);
            c.emplace_back(-var->signBit());
            addClause(c);
        }
    }
    return answer;
}
bvar blaster_solver::MultiplyBitInt(bvar var, literal bit, unsigned len){
    bvar answer = mkHolder("auxVar_submuli("+to_string(auxSize++)+")", len);
    Escape(answer);
    if(answer->size()<=var->size()){
        for(size_t i=1;i<answer->size();i++){
            if(bit==1){ // answer[i] == var[i]
                answer->setAt(i, var->getAt(i));
            }
            else{ // answer[i] = 0
                answer->setAt(i, solver->False());
            }
        }
    }
    else{ // answer->size() > var->size()
        for(size_t i=1;i<var->size();i++){
            if(bit==1){ // answer[i] == var[i]
                answer->setAt(i, var->getAt(i));
            }
            else{ // answer[i] = 0
                answer->setAt(i, solver->False());
            }
        }
        // extend var's sing bit.
        for(size_t i=var->size();i<answer->size();i++){
            if(bit==1){ // answer[i] == var[i]
                answer->setAt(i, var->signBit());
            }
            else{ // answer[i] = ~var[i]
                answer->setAt(i, solver->False());
            }
        }
    }
    return answer;
}
bvar blaster_solver::ShiftAdd(bvar subsum, bvar subans, bvar answer, unsigned idx){
    // directly set subsum[1] == answer[index]
    innerEqualBit(answer->getAt(idx-1), subsum->getAt(1));
    unsigned size = answer->size()-idx;
    bvar nextsubsum = mkInnerVar("auxVar_subsum("+to_string(auxSize++)+")", size);
    bvar nextsubcry = mkInnerVar("auxVar_subcry("+to_string(auxSize++)+")", size);
    size += 1; // nextsubsum->size(), debug: 2021.11.01
    // nextsubcry[1] == false; nextsubcry[0], nextsubsum[0] is not important
    clause c;
    c.emplace_back(-nextsubcry->getAt(1));
    addClause(c);
    for(size_t i=1;i<size-1;i++){
        doTarget(nextsubsum->getAt(i), subans->getAt(i), subsum->getAt(i+1), nextsubcry->getAt(i));
        doCarry(nextsubcry->getAt(i+1), subans->getAt(i), subsum->getAt(i+1), nextsubcry->getAt(i));
    }
    doTarget(nextsubsum->getAt(size-1), subans->getAt(size-1), subsum->getAt(size), nextsubcry->getAt(size-1));
    return nextsubsum;
}
bvar blaster_solver::Invert(bvar var){ return mkInvertedVar(var); }
bvar blaster_solver::AddOne(bvar var){ return Add(var, mkInt(1), true); }
bvar blaster_solver::Shift(bvar var, int len){
    // debug: 2021-11-06: len can not to unsigned.
    if(len>(int)var->csize()) return mkInt(0);
    else if(len==0) return var;
    else return mkShiftedVar(var, len);
}

// inner auxiliary functions for math operations
void blaster_solver::doCarry (literal cry1, literal varmax, literal varmin, literal cry0){
    if(varmax == solver->False()){
        doCarryInt(cry1, varmin, 0, cry0);
        return;
    }
    else if(varmin == solver->False()){
        doCarryInt(cry1, varmax, 0, cry0);
        return;
    }
    else if(cry0 == solver->False()){
        doCarryInt(cry1, varmax, 0, varmin);
        return;
    }
    else if(varmax==varmin){ // e.g. signbit in x + x<<1
        clause c;
        // once varmax == varmin == 1, then cry1 == 1
        // once varmax == varmin == 1, then cry1 will never == 1.
        // ~carry[i+1] \/ varmax[i] \/ varmin[i]
        c.clear();
        c.emplace_back(-cry1); 
        c.emplace_back(varmin);
        addClause(c);
        // carry[i+1] \/ ~varmax[i] \/ ~varmin[i]
        c.clear();
        c.emplace_back(cry1); 
        c.emplace_back(-varmin);
        addClause(c);

        return;
    }
    clause c;
    // carry -> ALL
    // ~carry[i+1] \/ varmax[i] \/ varmin[i] \/ carry[i]
    // c.clear();
    // c.emplace_back(-cry1); 
    // c.emplace_back(varmax); 
    // c.emplace_back(varmin); 
    // c.emplace_back(cry0); 
    // addClause(c);
    // ~carry[i+1] \/ varmax[i] \/ varmin[i]
    c.clear();
    c.emplace_back(-cry1); 
    c.emplace_back(varmax); 
    c.emplace_back(varmin);
    addClause(c);
    // ~carry[i+1] \/ varmax[i] \/ carry[i]
    c.clear();
    c.emplace_back(-cry1); 
    c.emplace_back(varmax);
    c.emplace_back(cry0); 
    addClause(c);
    // ~carry[i+1] \/ varmin[i] \/ carry[i]
    c.clear();
    c.emplace_back(-cry1);
    c.emplace_back(varmin); 
    c.emplace_back(cry0); 
    addClause(c);
    
    // ALL -> carry
    // carry[i+1] \/ ~varmax[i] \/ ~varmin[i]
    c.clear();
    c.emplace_back(cry1); 
    c.emplace_back(-varmax); 
    c.emplace_back(-varmin);
    addClause(c);
    // carry[i+1] \/ ~varmax[i] \/ ~carry[i]
    c.clear();
    c.emplace_back(cry1); 
    c.emplace_back(-varmax);
    c.emplace_back(-cry0); 
    addClause(c);
    // carry[i+1] \/ ~varmin[i] \/ ~carry[i]
    c.clear();
    c.emplace_back(cry1);
    c.emplace_back(-varmin); 
    c.emplace_back(-cry0); 
    addClause(c);
    // carry[i+1] \/ ~varmax[i] \/ ~varmin[i] \/ ~carry[i]
    c.clear();
    c.emplace_back(cry1);
    c.emplace_back(-varmax); 
    c.emplace_back(-varmin); 
    c.emplace_back(-cry0); 
    addClause(c);
}
void blaster_solver::doTarget(literal target, literal varmax, literal varmin, literal cry){
    if(varmax == solver->False()){
        doTargetInt(target, varmin, 0, cry);
        return;
    }
    else if(varmin == solver->False()){
        doTargetInt(target, varmax, 0, cry);
        return;
    }
    else if(cry == solver->False()){
        doTargetInt(target, varmax, 0, varmin);
        return;
    }
    else if(varmax==varmin){ // signbit in x + x<<1
        clause c;
        // ~target[i] \/ varmax[i] \/ varmin[i] \/ carry[i]
        c.clear();
        c.emplace_back(-target); 
        c.emplace_back(varmax); 
        c.emplace_back(cry); 
        addClause(c);
        // ~target[i] \/ ~varmax[i] \/ ~varmin[i] \/ carry[i]
        c.clear();
        c.emplace_back(-target); 
        c.emplace_back(-varmax); 
        c.emplace_back(cry); 
        addClause(c);
        // target[i] \/ varmax[i] \/ varmin[i] \/ ~carry[i]
        c.clear();
        c.emplace_back(target); 
        c.emplace_back(varmax); 
        c.emplace_back(-cry); 
        addClause(c);
        // target[i] \/ ~varmax[i] \/ ~varmin[i] \/ ~carry[i]
        c.clear();
        c.emplace_back(target); 
        c.emplace_back(-varmax); 
        c.emplace_back(-cry); 
        addClause(c);

        return;
    }
    // target <-> varmax xor varmin xor cry
    clause c;
    // target -> ALL
    // ~target[i] \/ varmax[i] \/ varmin[i] \/ carry[i]
    c.clear();
    c.emplace_back(-target); 
    c.emplace_back(varmax); 
    c.emplace_back(varmin); 
    c.emplace_back(cry); 
    addClause(c);
    // ~target[i] \/ varmax[i] \/ ~varmin[i] \/ ~carry[i]
    c.clear();
    c.emplace_back(-target); 
    c.emplace_back(varmax); 
    c.emplace_back(-varmin); 
    c.emplace_back(-cry); 
    addClause(c);
    // ~target[i] \/ ~varmax[i] \/ varmin[i] \/ ~carry[i]
    c.clear();
    c.emplace_back(-target); 
    c.emplace_back(-varmax); 
    c.emplace_back(varmin); 
    c.emplace_back(-cry); 
    addClause(c);
    // ~target[i] \/ ~varmax[i] \/ ~varmin[i] \/ carry[i]
    c.clear();
    c.emplace_back(-target); 
    c.emplace_back(-varmax); 
    c.emplace_back(-varmin); 
    c.emplace_back(cry); 
    addClause(c);
    
    // ALL -> target
    // target[i] \/ varmax[i] \/ varmin[i] \/ ~carry[i]
    c.clear();
    c.emplace_back(target); 
    c.emplace_back(varmax); 
    c.emplace_back(varmin); 
    c.emplace_back(-cry); 
    addClause(c);
    // target[i] \/ varmax[i] \/ ~varmin[i] \/ carry[i]
    c.clear();
    c.emplace_back(target); 
    c.emplace_back(varmax); 
    c.emplace_back(-varmin); 
    c.emplace_back(cry); 
    addClause(c);
    // target[i] \/ ~varmax[i] \/ varmin[i] \/ carry[i]
    c.clear();
    c.emplace_back(target); 
    c.emplace_back(-varmax); 
    c.emplace_back(varmin); 
    c.emplace_back(cry); 
    addClause(c);
    // target[i] \/ ~varmax[i] \/ ~varmin[i] \/ ~carry[i]
    c.clear();
    c.emplace_back(target); 
    c.emplace_back(-varmax); 
    c.emplace_back(-varmin); 
    c.emplace_back(-cry); 
    addClause(c);
}
void blaster_solver::doCarryInt (literal cry1, literal var, literal ivar, literal cry0){
    clause c;
    if(ivar==0){
        // ~cry1 \/ var
        c.clear();
        c.emplace_back(-cry1); 
        c.emplace_back(var);
        addClause(c);

        // ~cry1 \/ cry0
        c.clear();
        c.emplace_back(-cry1);
        c.emplace_back(cry0); 
        addClause(c);

        // cry1 \/ ~var \/ ~cry0
        c.clear();
        c.emplace_back(cry1); 
        c.emplace_back(-var);
        c.emplace_back(-cry0); 
        addClause(c);
    }
    else{ // ivar = 1
        // ~cry1 \/ var \/ cry0
        c.clear();
        c.emplace_back(-cry1); 
        c.emplace_back(var);
        c.emplace_back(cry0); 
        addClause(c);
        
        // cry1 \/ ~var
        c.clear();
        c.emplace_back(cry1); 
        c.emplace_back(-var);
        addClause(c);

        // cry1 \/ ~cry0
        c.clear();
        c.emplace_back(cry1);
        c.emplace_back(-cry0); 
        addClause(c);
    }
}
void blaster_solver::doTargetInt(literal target, literal var, literal ivar, literal cry){
    clause c;
    if(ivar==1){ 
        // ~target \/ var \/ ~cry
        c.clear();
        c.emplace_back(-target); 
        c.emplace_back(var);
        c.emplace_back(-cry); 
        addClause(c);

        // ~target \/ ~var \/ cry
        c.clear();
        c.emplace_back(-target); 
        c.emplace_back(-var);
        c.emplace_back(cry); 
        addClause(c);

        // target \/ ~var \/ ~cry
        c.clear();
        c.emplace_back(target); 
        c.emplace_back(-var); 
        c.emplace_back(-cry); 
        addClause(c);

        // target \/ var \/ cry
        c.clear();
        c.emplace_back(target); 
        c.emplace_back(var);
        c.emplace_back(cry); 
        addClause(c);
    }
    else{ // ivar = 0
        // ~target \/ ~var \/ ~cry
        c.clear();
        c.emplace_back(-target); 
        c.emplace_back(-var); 
        c.emplace_back(-cry); 
        addClause(c);

        // ~target \/ var \/ cry
        c.clear();
        c.emplace_back(-target); 
        c.emplace_back(var);
        c.emplace_back(cry); 
        addClause(c);

        // target \/ ~var \/ cry
        c.clear();
        c.emplace_back(target); 
        c.emplace_back(-var);
        c.emplace_back(cry); 
        addClause(c);
        
        // target \/ var \/ ~cry
        c.clear();
        c.emplace_back(target); 
        c.emplace_back(var);
        c.emplace_back(-cry); 
        addClause(c);
    }
}

