/* searcher.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "qfnia/searcher.hpp"

using namespace ismt;

#define assertDebug 0
#define statusDebug 0

Searcher::Searcher(){
    solver = new blaster_solver();
    transformer = new blaster_transformer(solver);
}
Searcher::~Searcher(){
    delete transformer; transformer = nullptr;
    delete solver; solver = nullptr;
}

void Searcher::init(Info* i){
    info = i;
    // declare the assigned variable
    std::vector<dagc*> vars;
    info->message->model->getAssignedVars(vars);
    for(unsigned i=0;i<vars.size();i++){
        _declare(vars[i]);
    }
    transformer->GA = info->options->GA;
}

void Searcher::_declare(dagc* root){
    if(root->isvnum()){
        if(root->isAssigned()){
            transformer->_declare(root, root->v, false, root->v, false);
        }
        else{
            Interval i(info->assignment[root]);
            transformer->_declare(root, 
                to_long(getLower(i)), 
                poly::get_lower_open(i), 
                to_long(getUpper(i)), 
                poly::get_upper_open(i));
        }
    }
    else if(root->isvbool()){
        transformer->_declare(root);
    }
}
int Searcher::_assert(dagc* root){
    return transformer->_assert(root);
}
bool Searcher::search(){
    // check whether it is needed to re-transform
    // if not modify the former constraints and not change the assignment
    // i.e. append / delete constraints -> not re-transform
    // i.e. not resolve
    for(unsigned i=0;i<info->new_vars.size();i++){
        _declare(info->new_vars[i]);
    }
    if(info->new_constraints.size() == 0) return true;
    #if assertDebug
        std::cout<<"----------------------searching----------------------"<<std::endl;
    #endif
    for(unsigned i=0;i<info->new_constraints.size();i++){
        #if assertDebug
            info->message->data->printAST(info->new_constraints[i]);
            std::cout<<std::endl;
        #endif
        info->constraints.emplace_back(info->new_constraints[i]);
        info->assumptions.emplace_back(_assert(info->new_constraints[i]));
    }
    info->new_vars.clear();
    info->new_constraints.clear();
    #if statusDebug
        solver->printStatus();
    #endif

    // solve
    bool ans = solver->solve(info->assumptions);
    if(ans){
        info->state = State::SAT;
        return true;
    }
    return false;
}
void Searcher::set_model(){
    for(size_t i=0;i<info->variables.size();i++){
        dagc* var = info->variables[i];
        info->message->model->set(var, solver->getValue(var->name));
    }
}
void Searcher::reset(){
    solver->reset();
    transformer->reset();
    // declare the assigned variable
    std::vector<dagc*> vars;
    info->message->model->getAssignedVars(vars);
    for(unsigned i=0;i<vars.size();i++){
        _declare(vars[i]);
    }
}

