/* sat_solver.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/sat/sat_solver.hpp"
#include <iostream>

using namespace ismt;
#define printClauses 0
#define printModel 0

sat_solver::sat_solver(std::string n){
    name = n;
    state = State::UNKNOWN;
    solver = new CaDiCaL::Solver();

    setConst();
}
sat_solver::~sat_solver(){
    delete solver;
    solver = nullptr;
}
void sat_solver::setConst(){
    solver->set("log", 1);
    SAT_False = 1, SAT_True = 2, iVars = 3;
    // false literal
    solver->add(-1);
    solver->add(0);
    // true literal
    solver->add(2);
    solver->add(0);
    iClauses = 2;
}
bool_var sat_solver::False() const {
    return SAT_False;
}
bool_var sat_solver::True() const {
    return SAT_True;
}
bool_var sat_solver::curVar() const{
    return iVars;
}

// number of vars
int sat_solver::nVars() const{
    return iVars-1;
}
// number of clauses
int sat_solver::nClauses() const{
    return iClauses;
}

// add to solver
int sat_solver::newVar(){
    return iVars++;
}
int sat_solver::newVars(unsigned num){
    iVars+=num;
    return iVars;
}
bool sat_solver::addClause(clause& c){
    
    #if printClauses
        std::cout<<"add clause: ";
        for(size_t i=0;i<c.size();i++){
            std::cout<<c[i]<<" ";
        }
        std::cout<<"0"<<std::endl;
    #endif

    for(size_t i = 0;i<c.size();i++){
        solver->add(c[i]);
    }
    solver->add(0);
    ++iClauses;
    
    return true;
}

// solve
bool sat_solver::simplify(const literals& c){
    return solve(c);
}
bool sat_solver::solve(){
    int ans = solver->solve();
    if(ans==10) state = State::SAT;
    else if(ans==20) state = State::UNSAT;
    else state = State::UNKNOWN;
    return true;
}
bool sat_solver::solve(const literals& aps){
    assumptions.clear();
    for(size_t i=0;i<aps.size();i++){
        assumptions.emplace_back(aps[i]);
        solver->assume(aps[i]);
    }
    return solve();
}

// state
State sat_solver::getState() const{
    return state;
}

// model
model sat_solver::getModel(){
    model t;
    
    if(state==State::SAT){
        t.emplace_back(0); // place holder for index of 0
        for(unsigned i=1;i<iVars;i++){
            t.emplace_back(solver->val(i)>0?1:0);
            #if printModel
                std::cout<<i<<": "<<t[i]<<std::endl;
            #endif
        }
        return t;
    }
    else if(state==State::UNKNOWN){
        std::cout<<"Can not get model, because state is State::UNKNOWN!\n";
    }
    else{
        std::cout<<"Can not get model, because state is State::UNSAT!\n";
    }
    return t;
}


// reset
void sat_solver::reset(){
    delete solver; solver = nullptr;
    solver = new CaDiCaL::Solver();
    setConst();
}


// benchmark setting
void sat_solver::setBenchmark(std::string file){
    benchmark = file;
}


// cdcl(t) function
bool sat_solver::propagate(){
    solver->limit("decisions", 0);
    return solver->solve();
}

void sat_solver::partial(boost::unordered_map<int, int>& m){
    for(size_t i=1;i<iVars;i++){
        if(val(i)==(int)i){
            m.insert(std::pair<int, int>(i, i));
        }
        else if(val(i)==-(int)i){
            m.insert(std::pair<int, int>(i, -i));
        }
    }
}
int sat_solver::val(int lit){
    return solver->val(lit);
}

// get unsat assumptions
void sat_solver::getConflict(literals& aps){
    for(size_t i=0;i<assumptions.size();i++){
        if(solver->failed(assumptions[i])){
            aps.emplace_back(assumptions[i]);
        }
    }
}