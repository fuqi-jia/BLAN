/* blaster_recursion.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"

using namespace ismt;


// Recursion Abstraction: e.g. a > b <=> a[1] > a[2] or a[1] == a[2] /\ a[2:] > b[2:]
literal blaster_solver::REqual(bvar var1, bvar var2){
    return EqualVar(var1, var2);
}
literal blaster_solver::RNotEqual(bvar var1, bvar var2){
    literals lits;
    unsigned len = min(var1->size(), var2->size());
    for(size_t i=0;i<len;i++){
        lits.emplace_back(NotEqualBit(var1->getAt(i), var2->getAt(i)));
    }
    for(size_t i=len;i<var1->size();i++){
        // var2 extend sign bit
        lits.emplace_back(NotEqualBit(var1->getAt(i), var2->signBit()));
    }
    for(size_t i=len;i<var2->size();i++){
        // var1 extend sign bit
        lits.emplace_back(NotEqualBit(var2->getAt(i), var1->signBit()));
    }

    return Or(lits);
}
literal blaster_solver::RGreaterBit(const literal& var1, const literal& var2){

    bool_var t = newSatVar();
    // t == var1[i] > var2[i] <=> var1[i] = 0 & var2[i] = 1.
    clause c;
    // t -> ~var1[i] /\ var2[i]
    c.clear();
    c.emplace_back(-t);
    c.emplace_back(-var1);
    addClause(c);

    c.clear();
    c.emplace_back(-t);
    c.emplace_back(var2);
    addClause(c);

    // ~var1[i] /\ var2[i] -> t
    c.clear();
    c.emplace_back(-t);
    c.emplace_back(var1);
    c.emplace_back(-var2);
    addClause(c);

    return t;
}
literal blaster_solver::RGreater(bvar var1, bvar var2){ // var1 > var2

    bool_var t = newSatVar();

    // var1 >=0 && var2 < 0 -> t
    // v1zv2 <-> var1 >=0 && var2 < 0
    // ~var1[0] /\ var2[0] -> v1zv2 <=> var1[0] \/ ~var2[0] \/ v1zv2
    bool_var v1zv2 = newSatVar();
    clause c;
    c.emplace_back(var1->signBit());
    c.emplace_back(-var2->signBit());
    c.emplace_back(v1zv2);
    addClause(c);
    // ~var1[0] /\ var2[0] <- v1zv2 <=> ~var1[0] \/ ~v1zv2 /\ var2[0] \/ ~v1zv2
    c.clear();
    c.emplace_back(-var1->signBit());
    c.emplace_back(-v1zv2);
    addClause(c);

    c.clear();
    c.emplace_back(var2->signBit());
    c.emplace_back(-v1zv2);
    addClause(c);

    unsigned len = min(var1->size(),var2->size());
    bool_var loopi=0;
    literals lits;
    // var1 < 0 && var2 < 0 && (var1[1:] > var2[1:]) -> zv1v2
    for(size_t i=1;i<len;i++){
        // loopi == var1[i-1] > var2[i-1];
        // tmp == var1[i] > var2[i]; 

        // (var1[1:] > var2[1:]) <=> var1[i] = 0 && var2[2] = 1
        bool_var tmp = RGreaterBit(var2->getAt(i), var1->getAt(i));
        if(i!=1){
            lits.clear();
            // var1[i] == var2[i]
            lits.emplace_back(EqualBit(var1->getAt(i), var2->getAt(i)));
            lits.emplace_back(loopi);
            // loopi == var1[i] > var2[i] \/ (var1[i]==var2[i] /\ var1[i-1] > var2[i-1]), 2<=i<len.
            loopi = Or(tmp, And(lits));
        }
        else loopi = tmp; // var1[1] > var2[1]
    }
    for(size_t i=len;i<var1->size();i++){ // var1->size() > var2->size()
        bool_var tmp = RGreaterBit(var2->signBit(), var1->getAt(i));

        lits.clear();
        // var1[i] == var2[i]
        lits.emplace_back(EqualBit(var1->getAt(i), var2->signBit()));
        lits.emplace_back(loopi);

        loopi = Or(tmp, And(lits));
    }
    for(size_t i=len;i<var2->size();i++){ // var1->size() < var2->size()
        bool_var tmp = RGreaterBit(var2->getAt(i), var1->signBit());

        lits.clear();
        // var1[i] == var2[i]
        lits.emplace_back(EqualBit(var1->signBit(), var2->getAt(i)));
        lits.emplace_back(loopi);

        loopi = Or(tmp, And(lits));
    }
    lits.clear();
    lits.emplace_back(LtZero(var1));
    lits.emplace_back(LtZero(var2));
    lits.emplace_back(loopi);
    bool_var zv1v2 = And(lits);
    
    loopi=0;
    // var1 > 0 && var2 > 0 && (var1[1:] > var2[1:]) -> v1v2z
    for(size_t i=1;i<len;i++){
        // loopi == var1[i-1] > var2[i-1];
        // tmp == var1[i] > var2[i]; 

        // (var1[1:] > var2[1:]) <=> var1[i] = 1 && var2[i] = 0
        bool_var tmp = RGreaterBit(var1->getAt(i), var2->getAt(i));
        if(i!=1){
            lits.clear();
            // var1[i] == var2[i]
            lits.emplace_back(EqualBit(var1->getAt(i), var2->getAt(i)));
            lits.emplace_back(loopi);
            // loopi == var1[i] > var2[i] \/ (var1[i]==var2[i] /\ var1[i-1] > var2[i-1]), 2<=i<len.
            loopi = Or(tmp, And(lits));
        }
        else loopi = tmp; // var1[1] > var2[1]
    }
    for(size_t i=len;i<var1->size();i++){ // var1->size() > var2->size()
        bool_var tmp = RGreaterBit(var1->getAt(i), var2->signBit());

        lits.clear();
        // var1[i] == var2[i]
        lits.emplace_back(EqualBit(var1->getAt(i), var2->signBit()));
        lits.emplace_back(loopi);

        loopi = Or(tmp, And(lits));
    }
    for(size_t i=len;i<var2->size();i++){ // var1->size() < var2->size()
        bool_var tmp = RGreaterBit(var1->signBit(), var2->getAt(i));

        lits.clear();
        // var1[i] == var2[i]
        lits.emplace_back(EqualBit(var1->signBit(), var2->getAt(i)));
        lits.emplace_back(loopi);

        loopi = Or(tmp, And(lits));
    }
    lits.clear();
    lits.emplace_back(GtZero(var1));
    lits.emplace_back(GtZero(var2));
    lits.emplace_back(loopi);
    bool_var v1v2z = And(lits);

    // t <-> v1zv2 \/ zv1v2 \/ v1v2z.
    lits.clear();
    lits.emplace_back(v1zv2);
    lits.emplace_back(zv1v2);
    lits.emplace_back(v1v2z);
    innerEqualBit(t, Or(lits));

    return t;
}
literal blaster_solver::RGreaterEqual(bvar var1, bvar var2){ // var1 >= var2
    literals lits;
    lits.emplace_back(RGreater(var1, var2));
    lits.emplace_back(REqual(var1, var2));
    return Or(lits);
}
literal blaster_solver::REqualInt(bvar var, bvar v){
    if(var->isConstant()) swap(var, v);
    return EqualInt(var, v);
}
literal blaster_solver::RNotEqualInt(bvar var, bvar v){
    if(var->isConstant()) swap(var, v);
    return NotEqualInt(var, v);
}

literal blaster_solver::RGreaterBitInt(const literal& var, const literal& v){
    // var > v: var = 1 and v = 0.
    if(v==1) return solver->False();
    else return var;
}

literal blaster_solver::RLessBitInt(const literal& var, const literal& v){
    // var < v: var = 0 and v = 1.
    if(v==1) return -var;
    else return solver->False();
}

literal blaster_solver::RCompIntLoop(bvar var, bvar v, bool greater){
    unsigned len = min(var->size(),v->size());
    bool_var loopi = solver->False();
    literals lits;
    if((greater&&v->signBit()==0) || (!greater&&v->signBit()==1)){
        // var > v && v >= 0 || var < v && v < 0 ( var < 0 )
        for(size_t i=1;i<len;i++){
            bool_var tmp = RGreaterBitInt(var->getAt(i), v->getAt(i));

            if(i!=1){
                lits.clear();
                // var[i] == v[i]
                lits.emplace_back(EqualBitInt(var->getAt(i), v->getAt(i)));
                lits.emplace_back(loopi);
                // loopi == var1[i] > var2[i] \/ (var1[i]==var2[i] /\ var1[i-1] > var2[i-1]), 2<=i<len.
                loopi = Or(tmp, And(lits));
            }
            else loopi = tmp;
        }

        // if var->size() < v->size()
        for(size_t i=len;i<v->size();i++){
            bool_var tmp = RGreaterBitInt(var->signBit(), v->getAt(i));

            lits.clear();
            // var1[i] == var2[i]
            lits.emplace_back(EqualBitInt(var->signBit(), v->getAt(i)));
            lits.emplace_back(loopi);

            loopi = Or(tmp, And(lits));
        }
        // if var->size() >= v->size()
        for(size_t i=len;i<var->size();i++){
            bool_var tmp = RGreaterBitInt(var->getAt(i), v->signBit());

            lits.clear();
            // var[i] == v[i]
            lits.emplace_back(EqualBitInt(var->getAt(i), v->signBit()));
            lits.emplace_back(loopi);

            loopi = Or(tmp, And(lits));
        }
        // if(v->signBit()==1){ // v < 0 
        //     // v extend sign bit (1), var < v means all False.
        // }
        // else{ // v >= 0
        //     for(size_t i=len;i<var->size();i++){
        //         bool_var tmp = RGreaterBitInt(var->getAt(i), v->signBit());

        //         lits.clear();
        //         // var[i] == v[i]
        //         lits.emplace_back(EqualBitInt(var->getAt(i), v->signBit()));
        //         lits.emplace_back(loopi);

        //         loopi = Or(tmp, And(lits));
        //     }
        // }
    }
    else if((!greater&&v->signBit()==0) || (greater&&v->signBit()==1)){
        // var < v && v >= 0 || var > v && v < 0 ( var < 0 )
        for(size_t i=1;i<len;i++){
            bool_var tmp = RLessBitInt(var->getAt(i), v->getAt(i));

            if(i!=1){
                lits.clear();
                // var[i] == v[i]
                lits.emplace_back(EqualBitInt(var->getAt(i), v->getAt(i)));
                lits.emplace_back(loopi);
                // loopi == var1[i] > var2[i] \/ (var1[i]==var2[i] /\ var1[i-1] > var2[i-1]), 2<=i<len.
                loopi = Or(tmp, And(lits));
            }
            else loopi = tmp;
        }
        
        // if var->size() < v->size()
        for(size_t i=len;i<v->size();i++){
            bool_var tmp = RLessBitInt(var->signBit(), v->getAt(i));

            lits.clear();
            // var1[i] == var2[i]
            lits.emplace_back(EqualBitInt(var->signBit(), v->getAt(i)));
            lits.emplace_back(loopi);

            loopi = Or(tmp, And(lits));
        }
        // if var->size() > v->size()
        for(size_t i=len;i<var->size();i++){
            bool_var tmp = RLessBitInt(var->getAt(i), v->signBit());

            lits.clear();
            // var[i] == v[i]
            lits.emplace_back(EqualBitInt(var->getAt(i), v->signBit()));
            lits.emplace_back(loopi);

            loopi = Or(tmp, And(lits));
        }
        // if(v->signBit()==1){ // v < 0 
        //     for(size_t i=len;i<var->size();i++){
        //         bool_var tmp = RLessBitInt(var->getAt(i), v->signBit());

        //         lits.clear();
        //         // var[i] == v[i]
        //         lits.emplace_back(EqualBitInt(var->getAt(i), v->signBit()));
        //         lits.emplace_back(loopi);

        //         loopi = Or(tmp, And(lits));
        //     }
        // }
        // else{ // v >= 0
        //     // v extend sign bit (0), var < v means all False.
        // }
    }
    return loopi;
}
literal blaster_solver::RLessInt(bvar var, bvar v){
    // var < v
    bool_var loopi = RCompIntLoop(var, v, false);
    literals lits;
    if(v->signBit()==1){ // v < 0
        // var < 0 /\ loopi
        lits.clear();
        lits.emplace_back(LtZero(var));
        lits.emplace_back(loopi);
        return And(lits);
    }
    else{ // v >= 0
        // var < 0 \/ var >= 0 /\ loopi
        lits.clear();
        lits.emplace_back(GeZero(var));
        lits.emplace_back(loopi);
        bool_var gez = And(lits);

        lits.clear();
        lits.emplace_back(gez);
        lits.emplace_back(LtZero(var));
        return Or(lits);
    }
}
literal blaster_solver::RGreaterInt(bvar var, bvar v){ // var > v

    if(var->isConstant()){ // swap, var < v
        swap(var, v);
        return RLessInt(var, v);
    }
    else{
        bool_var loopi = RCompIntLoop(var, v, true);
        literals lits;
        if(v->signBit()==1){ // v < 0
            // var >= 0 \/ var < 0 /\ loopi.
            lits.clear();
            lits.emplace_back(LtZero(var));
            lits.emplace_back(loopi);
            bool_var ltz = And(lits);

            lits.clear();
            lits.emplace_back(GeZero(var));
            lits.emplace_back(ltz);
            return Or(lits);
        }
        else{ // v >= 0
            // var >= 0 /\ loopi 
            lits.clear();
            lits.emplace_back(GeZero(var));
            lits.emplace_back(loopi);
            return And(lits);
        }
    }
}
literal blaster_solver::RGreaterEqualInt(bvar var, bvar v){ // var >= v
    literals lits;
    lits.emplace_back(RGreaterInt(var, v));
    lits.emplace_back(REqualInt(var, v));
    return Or(lits);
}
