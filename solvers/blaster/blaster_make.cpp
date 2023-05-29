/* blaster_make.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"

using namespace ismt;

// ------------make variables------------

// make blast_variables
bvar blaster_solver::mkVar(const std::string& name){
    return mkVar(name, default_len);
}
bvar blaster_solver::mkVar(const std::string& name, unsigned len){
    bvar t = blast(mkHolder(name, len));
    Escape(t);
    return t;
}
bvar blaster_solver::mkInnerVar(const std::string& name, unsigned len){
    if(n2var.find(name)!=n2var.end()){
        bvar x = n2var[name];
        if(x->isClean()){
            x->setSize(len);
            x->reblast();
            blast(x);
            // debug: 2021-11-12, must Escape.
            Escape(x);
            return x;
        }
        else return x;
    }
    bvar t = blast(new blast_variable(name, len));
    // add to map, for future delete.
    n2var.insert(std::pair<std::string, bvar>(name, t));
    Escape(t);
    return t;
}
bvar blaster_solver::mkHolder(const std::string& name, unsigned len){
    if(n2var.find(name)!=n2var.end()){
        bvar x = n2var[name];
        if(x->isClean()){
            x->setSize(len);
            x->reblast();
            return x;
        }
        else return x;
    }
    bvar t = new blast_variable(name, len);
    // add to map, for future delete.
    n2var.insert(std::pair<std::string, bvar>(name, t));
    ProblemVars.emplace_back(t);
    return t;
}
bvar blaster_solver::blast(bvar var){
    for(size_t i=0;i<var->size();i++){
        var->setAt(i, newSatVar());
    }
    return var;
}
void blaster_solver::addVar(bvar var){
    ProblemVars.emplace_back(var);
}
void blaster_solver::addVarMap(bvar var){
    n2var.insert(std::pair<std::string, bvar>(var->getName(), var));
}
void blaster_solver::copy(bvar var, std::string& name, bvar& target){
    if(target==nullptr){
        target = new blast_variable(name, var->csize());
        target->setKind(var->isConstant());
        target->setCurValue(var->getCurValue());
        n2var.insert(std::pair<std::string, bvar>(name, target));
        ProblemVars.emplace_back(target);
    }
    else target->setSize(var->csize());
    target->reblast();
    for(size_t i=0;i<target->size();i++){
        target->setAt(i, var->getAt(i));
    }
}
void blaster_solver::copyInt(bvar var, bvar& target){
    if(target==nullptr){
        target = new blast_variable(var->getName(), var->csize());
        target->setKind(var->isConstant());
        target->setCurValue(var->getCurValue());
    }
    else target->setSize(var->csize());
    target->reblast();
    for(size_t i=0;i<target->size();i++){
        target->setAt(i, var->getAt(i));
    }
}
void blaster_solver::Escape(bvar var){
    clause c;
    c.emplace_back(-var->signBit());
    for(size_t i=1;i<var->size();i++){
        c.emplace_back(var->getAt(i));
    }
    addClause(c);
}
bvar blaster_solver::mkInt(Integer v){
    if(n2int.find(v.get_str())!=n2int.end()){
        return n2int[v.get_str()];
    }
    std::string name = v.get_str();
    // special case
    if(v==0){
        bvar t = new blast_variable(name, 1, 0, true);
        n2int.insert(std::pair<std::string, bvar>(name, t));
        t->setAt(0, v>=0?0:1);
        t->setAt(1, 0);
        return t;
    }
    // calculate value
    int maxBits = 0;
    Integer val = abs(v);
    std::vector<int> answer;
    // true code of binary
    while(val!=0){
        answer.emplace_back(val%2==1?1:0);
        val/=2;
        maxBits ++;
    }
    if(v<0){
        // invert all bits
        for(size_t i=0;i<answer.size();i++){
            answer[i] = 1 - answer[i]; // 1->0, 0->1
        }
        // add one
        for(size_t i=0;i<answer.size();i++){
            if(answer[i]==1) answer[i] = 0;
            else{ answer[i]=1; break; }
        }
    }
    bvar t = new blast_variable(name, maxBits, v, true);
    n2int.insert(std::pair<std::string, bvar>(name, t));
    t->setAt(0, v>=0?0:1);
    for(size_t i=1;i<t->size();i++){
        t->setAt(i, answer[i-1]);
    }

    return t;
}
bool_var blaster_solver::mkBool(const std::string& name){
    bool_var t = newSatVar();
    n2bool.insert(std::pair<std::string, bool_var>(name, t));
    return t;
}

// auxiliary make variable functions
bvar blaster_solver::mkInvertedVar(bvar var){
    std::string name = "inv("+var->getName()+")";
    bvar t = mkHolder(name, var->csize());
    for(size_t i=0;i<t->size();i++){
        t->setAt(i, -var->getAt(i));
    }
    n2var.insert(std::pair<std::string, bvar>(t->getName(), t));
    return t;
}
bvar blaster_solver::mkShiftedVar(bvar var, int len){
    if(len==0) return var;
    // len>0: shift right, len<0: shift left.
    std::string name = "("+var->getName() + (len>0?">>":"<<") + to_string(abs(len))+")";
    bvar t = mkHolder(name, var->csize()-len);
    t->setAt(0, var->signBit());
    if(len>0){
       for(size_t i=1;i<t->size();i++){
            t->setAt(i, var->getAt(i+len));
        }
        Escape(t);
    }
    else{
        len = -len;
        for(int i=1;i<=len;i++) t->setAt(i, solver->False());
        for(size_t i=len+1;i<t->size();i++) t->setAt(i, var->getAt(i-len));
    }
    n2var.insert(std::pair<std::string, bvar>(name, t));
    return t;
}
bool_var blaster_solver::newSatVar(){
    bool_var t = solver->curVar();
    solver->newVar();
    return t;
}
