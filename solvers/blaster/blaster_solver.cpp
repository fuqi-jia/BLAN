/* blaster_solver.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"

#include <iostream>

using namespace ismt;

#define modelDebug 0

// ------------init and deinit------------
blaster_solver::blaster_solver(std::string n){
    solver = new sat_solver(n);
    default_len = 10;
    auxSize = 0;
    slackSize = 0;

    setvvEqMode(Recursion);
    setvvCompMode(Transposition);
    setviEqMode(Recursion);
    setviCompMode(Transposition);
    add2i = false;
}

blaster_solver::~blaster_solver(){
    clear();
}
void blaster_solver::reset(){
    clearVars();
    solver->reset();
}
void blaster_solver::clearVars(){
    boost::unordered_map<std::string, bvar>::iterator viter = n2var.begin();
    while(viter!=n2var.end()){
        if(!viter->second->isConstant()){
            viter->second->clear();
        }
        viter++;
    }
    n2bool.clear();
    EqZeroMap.clear();
}
void blaster_solver::clear(){
    boost::unordered_map<std::string, bvar>::iterator viter = n2var.begin();
    while(viter!=n2var.end()){
        delete viter->second;
        viter->second = nullptr;
        viter++;
    }
    boost::unordered_map<std::string, bvar>::iterator iiter = n2int.begin();
    while(iiter!=n2int.end()){
        delete iiter->second;
        iiter->second = nullptr;
        iiter++;
    }

    // TODO? error?
    delete solver;
    solver = nullptr;
}
void blaster_solver::setBenchmark(std::string file){
    benchmark = file;
    solver->setBenchmark(file);
}

// ------------output errors------------
void blaster_solver::outputError(const std::string& mesg){
    std::cout<<mesg<<"\n";
    assert(false);
}

// ------------add clause------------
void blaster_solver::addClause(clause& c){
    clause copy_lits(c);std::sort(copy_lits.begin(), copy_lits.end());
    clause ans;
    for(size_t i=0;i<copy_lits.size();i++){
        if(copy_lits[i]==solver->True()) return;
        if(copy_lits[i]==-solver->False()) return;
        else if(copy_lits[i]==solver->False()) continue;
        else if(copy_lits[i]==-solver->True()) continue;
        else if(i!=0&&copy_lits[i]==copy_lits[i-1]) continue;
        else ans.emplace_back(copy_lits[i]);
    }
    solver->addClause(ans);
}

// ------------solve operations------------
int blaster_solver::Assert(literal lit){
    literals lits { lit };
    solver->addClause(lits);
    return 1;
}
bool blaster_solver::solve(){
    solver->solve();
    if(State::SAT==solver->getState()) getModel();
    return State::SAT==solver->getState();
}
bool blaster_solver::solve(const literals& assumptions){
    if(assumptions.size() == 0) return true;
    solver->solve(assumptions);
    if(State::SAT==solver->getState()) getModel();
    return State::SAT==solver->getState();
}
bool blaster_solver::simplify(){
    literals c;
    return simplify(c);
}
bool blaster_solver::simplify(const literals& assumptions){
    solver->simplify(assumptions);
    if(State::SAT==solver->getState()) getModel();
    return State::UNSAT!=solver->getState();
}

// calculate value from bit-blasting
Integer blaster_solver::calculate(bvar var){
    int sign = mdl[abs(var->signBit())]?-1:1;
    #if modelDebug
        std::cout<<var->getName()<<": 0x"<<mdl[abs(var->signBit())];
    #endif
    Integer answer = 0;
    if(sign==-1){
        for(int i=var->size()-1;i>0;i--){
            answer = answer*2 + (mdl[abs(var->getAt(i))]?0:1);

            #if modelDebug
                std::cout<<(mdl[abs(var->getAt(i))]?0:1);
            #endif
        }
        answer += 1;
    }
    else{
        for(int i=var->size()-1;i>0;i--){
            answer = answer*2 + (mdl[abs(var->getAt(i))]?1:0);
            
            #if modelDebug
                std::cout<<(mdl[abs(var->getAt(i))]?1:0);
            #endif
        }
    }
    
    #if modelDebug
        std::cout<<"; value: "<<(sign==-1?-answer:answer)<<"."<<std::endl;
    #endif
    return sign==-1?-answer:answer;
}

// get model 
void blaster_solver::getModel(){
    assert(State::SAT==solver->getState());
    mdl = solver->getModel();
    for(size_t i=0;i<ProblemVars.size();i++){
        if(!ProblemVars[i]->isConstant()){
            ProblemVars[i]->setCurValue(calculate(ProblemVars[i]));
        }
    }
}

// ------------get operations------------
void blaster_solver::printModel(){
    std::cout<<"(model"<<std::endl;
    for(size_t i=0;i<ProblemVars.size();i++){
        std::cout<<"(define-fun "<<ProblemVars[i]->getName()<<" () Int "\
                 <<ProblemVars[i]->getCurValue()<<")\n";
    }
    
    boost::unordered_map<std::string, int>::iterator iter;
    iter = n2bool.begin();
    while(iter!=n2bool.end()){
        std::cout<<"(define-fun "<<iter->first<<" () Bool "\
                 <<(mdl[iter->second]?"true":"false")<<")\n";
        iter++;
    }
    std::cout<<")"<<std::endl;
}
void blaster_solver::printStatus(){
    std::cout<<"Number of variables: "<<solver->nVars()<<std::endl;
    std::cout<<"Number of clauses: "<<solver->nClauses()<<std::endl;
}
Integer blaster_solver::getValue(const bvar& var){
    return var->getCurValue();
}
Integer blaster_solver::getValue(const std::string& name){
    if(n2var.find(name)!=n2var.end()){
        bvar var = n2var[name];
        return var->getCurValue();
    }
    else if(n2bool.find(name)!=n2bool.end()){
        return (mdl[n2bool[name]]?1:0);
    }
    else { std::cout<<name<<std::endl; outputError("Error name for getting value!"); return 0;}
}

// ------------set operations------------
void blaster_solver::setDeaultLen(unsigned len){ default_len = len; }

literal blaster_solver::vvEq(bvar var1, bvar var2){
    if(vvEqMode==Slack) return SEqual(var1, var2);
    else if(vvEqMode==Recursion) return REqual(var1, var2);
    else return TEqual(var1, var2);
}
literal blaster_solver::vvGt(bvar var1, bvar var2){
    if(vvCompMode==Slack) return SGreater(var1, var2);
    else if(vvCompMode==Recursion) return RGreater(var1, var2);
    else return TGreater(var1, var2);
}
literal blaster_solver::vvGe(bvar var1, bvar var2){
    if(vvCompMode==Slack) return SGreaterEqual(var1, var2);
    else if(vvCompMode==Recursion) return RGreaterEqual(var1, var2);
    else return TGreaterEqual(var1, var2);
}
literal blaster_solver::vvNeq(bvar var1, bvar var2){
    if(vvEqMode==Slack) return SNotEqual(var1, var2);
    else if(vvEqMode==Recursion) return RNotEqual(var1, var2);
    else return TNotEqual(var1, var2);
}
literal blaster_solver::viEq(bvar var, bvar v){
    if(viEqMode==Slack) return SEqualInt(var, v);
    else if(viEqMode==Recursion) return REqualInt(var, v);
    else return TEqualInt(var, v);
}
literal blaster_solver::viGt(bvar var, bvar v){
    if(viCompMode==Slack) return SGreaterInt(var, v);
    else if(viCompMode==Recursion) return RGreaterInt(var, v);
    else return TGreaterInt(var, v);
}
literal blaster_solver::viGe(bvar var, bvar v){
    if(viCompMode==Slack) return SGreaterEqualInt(var, v);
    else if(viCompMode==Recursion) return RGreaterEqualInt(var, v);
    else return TGreaterEqualInt(var, v);
}
literal blaster_solver::viNeq(bvar var, bvar v){
    if(viEqMode==Slack) return SNotEqualInt(var, v);
    else if(viEqMode==Recursion) return RNotEqualInt(var, v);
    else return TNotEqualInt(var, v);
}
void blaster_solver::setvvEqMode(ConstraintsMode c){ vvEqMode = c; }
void blaster_solver::setvvCompMode(ConstraintsMode c){ vvCompMode = c; }
void blaster_solver::setviEqMode(ConstraintsMode c){ viEqMode = c; }
void blaster_solver::setviCompMode(ConstraintsMode c){ viCompMode = c; }

// ------------swap operations------------
void blaster_solver::swap(bvar& a, bvar& b){
    bvar t = a;
    a = b;
    b = t;
}

// ------------information operations------------
literal blaster_solver::Lit_True() { return solver->True(); }
literal blaster_solver::Lit_False() { return solver->False(); }
