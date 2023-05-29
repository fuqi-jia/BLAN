/* blaster_slack.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"

using namespace ismt;


// Slack Abstraction: e.g. a > b <=> a == b + t /\ t > 0.
literal blaster_solver::SEqual(bvar var1, bvar var2){ // var1 == var2
    bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var1->csize(), var2->csize()));
    bvar ans = Add(slack, var1);
    innerEqualVar(var2, ans);
    return EqZero(slack);
}
literal blaster_solver::SNotEqual(bvar var1, bvar var2){ // var1 != var2
    bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var1->csize(), var2->csize()));
    bvar ans = Add(slack, var1);
    innerEqualVar(var2, ans);
    return NeqZero(slack);
}
literal blaster_solver::SGreater(bvar var1, bvar var2){ // var1 > var2
    bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var1->csize(), var2->csize()));
    bvar ans = Add(slack, var2);
    innerEqualVar(var1, ans);
    return GtZero(slack);
}
literal blaster_solver::SGreaterEqual(bvar var1, bvar var2){ // var1 >= var2
    bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var1->csize(), var2->csize()));
    bvar ans = Add(slack, var2);
    innerEqualVar(var1, ans);
    return GeZero(slack);
}
literal blaster_solver::SEqualInt(bvar var1, bvar var2){
    bvar var = var1;
    bvar v   = var2;
    if(var->isConstant()) swap(var, v);
    if(v->getCurValue()==0){
        return EqZero(var);
    }
    bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var->csize(), v->csize()));
    if(add2i){
        bvar ans = Add(slack, v);
        innerEqualVar(var, ans);
        return EqZero(slack);
    }
    else{
        bvar ans = Add(slack, var);
        innerEqualInt(ans, v);
        return EqZero(slack);
    }
}
literal blaster_solver::SNotEqualInt(bvar var1, bvar var2){
    bvar var = var1;
    bvar v   = var2;
    if(var->isConstant()) swap(var, v);
    if(v->getCurValue()==0){
        return NeqZero(var);
    }
    bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var->csize(), v->csize()));
    if(add2i){
        bvar ans = Add(slack, v);
        innerEqualVar(var, ans);
        return NeqZero(slack);
    }
    else{
        bvar ans = Add(slack, var);
        innerEqualInt(ans, v);
        return NeqZero(slack);
    }
}
literal blaster_solver::SGreaterInt(bvar var1, bvar var2){ // var1 > var2
    if(var1->isConstant()){
        if(var1->getCurValue()==0){
            // var2 < 0 
            return LtZero(var2);
        }
        else{
            bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var1->csize(), var2->csize()));
            if(add2i){
                bvar ans = Add(slack, var1);
                innerEqualVar(var2, ans);
                // slack < 0.
                return LtZero(slack);
            }
            else{
                bvar ans = Add(slack, var2);
                innerEqualInt(ans, var1);
                // slack > 0.
                return GtZero(slack);
            }
        }
    }
    else{ // var2->isConstant()
        if(var2->getCurValue()==0){
            return GtZero(var1);
        }
        else{
            bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var1->csize(), var2->csize()));
            if(add2i){
                bvar ans = Add(slack, var2);
                innerEqualVar(var1, ans);
                // slack < 0.
                return GtZero(slack);
            }
            else{
                bvar ans = Add(slack, var1);
                innerEqualInt(ans, var2);
                // slack > 0.
                return LtZero(slack);
            }
        }
    }
}
literal blaster_solver::SGreaterEqualInt(bvar var1, bvar var2){ // var1 >= var2
    if(var1->isConstant()){
        if(var1->getCurValue()==0){
            // var2 <= 0 
            return LeZero(var2);
        }
        else{
            bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var1->csize(), var2->csize()));
            if(add2i){
                bvar ans = Add(slack, var1);
                innerEqualVar(var2, ans);
                // slack <= 0.
                return LeZero(slack);
            }
            else{
                bvar ans = Add(slack, var2);
                innerEqualInt(ans, var1);
                // slack >= 0.
                return GeZero(slack);
            }
        }
    }
    else{ // var2->isConstant()
        if(var2->getCurValue()==0){
            // vavr1 >= 0
            return GeZero(var1);
        }
        else{
            bvar slack = mkInnerVar("slack_"+to_string(slackSize++), max(var1->csize(), var2->csize()));
            if(add2i){
                bvar ans = Add(slack, var2);
                innerEqualVar(var1, ans);
                // slack <= 0.
                return GeZero(slack);
            }
            else{
                bvar ans = Add(slack, var1);
                innerEqualInt(ans, var2);
                // slack >= 0.
                return LeZero(slack);
            }
        }
    }
}

// auxiliary functions
literal blaster_solver::GtZero(bvar var){
    assert(!var->isConstant());
    // t <-> \/ var[i] /\ ~var[0]
    bool_var t = newSatVar();
    // t -> \/ var[i], i>=1 
    clause c;
    c.emplace_back(-t);
    for(size_t i=1;i<var->size();i++){
        c.emplace_back(var->getAt(i));
    }
    addClause(c);

    // t -> ~var[0]
    c.clear();
    c.emplace_back(-t);
    c.emplace_back(-var->signBit());
    addClause(c);

    // var[0] \/ ~var[i] \/ tmp, i>=1
    for(size_t i=1;i<var->size();i++){
        c.clear();
        c.emplace_back(t);
        c.emplace_back(var->signBit());
        c.emplace_back(-var->getAt(i));
        addClause(c);
    }

    return t;
}
literal blaster_solver::GeZero(bvar var){
    assert(!var->isConstant());
    // sign bit = 0 
    return -var->signBit();
}
literal blaster_solver::LtZero(bvar var){
    assert(!var->isConstant());
    // sign bit = 1 and Escape.
    return var->signBit();
}
literal blaster_solver::LeZero(bvar var){
    assert(!var->isConstant());
    // var < 0 \/ var == 0
    literals lits;
    lits.emplace_back(LtZero(var));
    lits.emplace_back(EqZero(var));
    return Or(lits);
}
literal blaster_solver::EqZero(bvar var){
    assert(!var->isConstant());
    if(EqZeroMap.find(var)!=EqZeroMap.end()) return EqZeroMap[var];
    // t <-> /\ ~var[i].
    bool_var t =  newSatVar();
    // t -> /\ ~var[i] <=> ~t \/ ~var[i], i>=0
    for(size_t i=0;i<var->size();i++){
        clause c;
        c.emplace_back(-t);
        c.emplace_back(-var->getAt(i));
        addClause(c);
    }

    // /\ ~var[i] -> t <=> \/ var[i] \/ t, i>=0
    clause c;
    c.emplace_back(t);
    for(size_t i=0;i<var->size();i++){
        c.emplace_back(var->getAt(i));
    }
    addClause(c);

    EqZeroMap.insert(std::pair<bvar, bool_var>(var, t));

    return t;
}
literal blaster_solver::NeqZero(bvar var){
    assert(!var->isConstant());
    // t <-> \/ var[i].
    bool_var t =  newSatVar();
    // t -> \/ var[i] <=> ~t \/ (\/ var[i]), i>=1
    clause c;
    c.emplace_back(-t);
    for(size_t i=1;i<var->size();i++){
        c.emplace_back(var->getAt(i));
    }
    addClause(c);

    // \/ var[i] -> t <=>  ~var[i] \/ t, i>=1
    for(size_t i=1;i<var->size();i++){
        c.clear();
        c.emplace_back(t);
        c.emplace_back(-var->getAt(i));
        addClause(c);
    }

    return t;
}