/* blaster_equal.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"

using namespace ismt;

// ------------Equal operations------------
void blaster_solver::innerEqualVar(bvar var1, bvar var2){
    clause c;
    unsigned len = min(var1->size(), var2->size());
    for(size_t i=0;i<len;i++){
        innerEqualBit(var1->getAt(i), var2->getAt(i));
    }
    // if var1->size() > var2->size()
    for(size_t i=len;i<var1->size();i++){
        innerEqualBit(var1->getAt(i), var2->signBit());
    }
    // if var1->size() <var2->size()
    for(size_t i=len;i<var2->size();i++){
        innerEqualBit(var1->signBit(), var2->getAt(i));
    }
}
void blaster_solver::innerEqualBit(literal var1, literal var2){
    clause c;
    c.emplace_back(-var1);
    c.emplace_back(var2);
    addClause(c);

    c.clear();
    c.emplace_back(var1);
    c.emplace_back(-var2);
    addClause(c);
}
void blaster_solver::innerEqualInt(bvar var, bvar v){
    clause c;
    unsigned len = min(var->size(), v->size());
    // if var->size() < v->size(), check whether can equal.
    if(var->size() < v->size()){
        literal tmp = v->getAt(len);
        for(size_t i=len;i<v->size();i++){
            // if v->getAt(i) != tmp means that this length of bits can not satisify the constraints.
            if(v->getAt(i)!=tmp){
                // TODO, can early stop.
                // this a range constraint for the variable!
                // IMPORTANT.
                c.emplace_back(solver->False());
                addClause(c);
            }
        }
        innerEqualBitInt(var->signBit(), tmp);
    }
    for(size_t i=0;i<len;i++){
        innerEqualBitInt(var->getAt(i), v->getAt(i));
    }
    // if var->size() > v->size()
    for(size_t i=len;i<var->size();i++){
        innerEqualBitInt(var->getAt(i), v->signBit());
    }
}
void blaster_solver::innerEqualBitInt(literal var, literal v){
    // var == v
    clause c;
    c.emplace_back(v==1?var:-var);
    addClause(c);
}
literal blaster_solver::EqualVar(bvar var1, bvar var2){
    literals lits;
    unsigned len = min(var1->size(), var2->size());
    for(size_t i=0;i<len;i++){
        lits.emplace_back(EqualBit(var1->getAt(i), var2->getAt(i)));
    }
    for(size_t i=len;i<var1->size();i++){
        // var2 extend sign bit
        lits.emplace_back(EqualBit(var1->getAt(i), var2->signBit()));
    }
    for(size_t i=len;i<var2->size();i++){
        // var1 extend sign bit
        lits.emplace_back(EqualBit(var2->getAt(i), var1->signBit()));
    }

    return And(lits);
}

literal blaster_solver::EqualBool(const literal& lit1, const literal& lit2){
    return EqualBit(lit1, lit2);
}

literal blaster_solver::NotEqualBool(const literal& lit1, const literal& lit2){
    return NotEqualBit(lit1, lit2);
}
literal blaster_solver::EqualBit(literal var1, literal var2){
    // t <-> var1 <-> var2
    bool_var t = newSatVar();

    // t -> var1 -> var2
    clause c;
    c.emplace_back(-t);
    c.emplace_back(-var1);
    c.emplace_back(var2);
    addClause(c);

    // t -> var2 -> var1
    c.clear();
    c.emplace_back(-t);
    c.emplace_back(var1);
    c.emplace_back(-var2);
    addClause(c);

    // var1 <-> var2 -> t
    // not (not var1 or var2 and not var2 or var1) or t
    // ( var1 /\ ~var2 \/ var2 /\ ~var1 ) \/ t
    // ( var1 \/ var2 \/ t ) /\ (~var1 \/ ~var2 \/ t)
    // ( var1 \/ var2 \/ t )
    c.clear();
    c.emplace_back(t);
    c.emplace_back(var1);
    c.emplace_back(var2);
    addClause(c);
    
    // (~var1 \/ ~var2 \/ t)
    c.clear();
    c.emplace_back(t);
    c.emplace_back(-var1);
    c.emplace_back(-var2);
    addClause(c);
    return t;
}
literal blaster_solver::EqualInt(bvar var, bvar v){
    if(v->getCurValue()==0) return EqZero(var);
    literals lits;
    unsigned len = min(var->size(), v->size());
    for(size_t i=0;i<len;i++){
        lits.emplace_back(EqualBitInt(var->getAt(i), v->getAt(i)));
    }
    for(size_t i=len;i<var->size();i++){
        // v extend sign bit
        lits.emplace_back(EqualBitInt(var->getAt(i), v->signBit()));
    }
    for(size_t i=len;i<v->size();i++){
        // var extend sign bit
        // TODO x(00) = 0100
        lits.emplace_back(EqualBitInt(var->signBit(), v->getAt(i)));
    }

    return And(lits);
}
literal blaster_solver::EqualBitInt(literal var, literal v){
    // var == v
    if(v==1) return var;
    else return -var;
}
literal blaster_solver::NotEqualVar(bvar var1, bvar var2){
    return -EqualVar(var1, var2);
}
literal blaster_solver::NotEqualBit(literal var1, literal var2){
    return -EqualBit(var1, var2);
}
literal blaster_solver::NotEqualInt(bvar var, bvar v){
    literals lits;
    unsigned len = min(var->size(), v->size());
    for(size_t i=0;i<len;i++){
        lits.emplace_back(NotEqualBitInt(var->getAt(i), v->getAt(i)));
    }
    for(size_t i=len;i<var->size();i++){
        // v extend sign bit
        lits.emplace_back(NotEqualBitInt(var->getAt(i), v->signBit()));
    }
    for(size_t i=len;i<v->size();i++){
        // var extend sign bit
        lits.emplace_back(NotEqualBitInt(var->signBit(), v->getAt(i)));
    }

    return Or(lits);
}
literal blaster_solver::NotEqualBitInt(literal var, literal v){
    // var == v
    if(v==1) return -var;
    else return var;
}