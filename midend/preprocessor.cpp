/* preprocessor.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "midend/preprocessor.hpp"

using namespace ismt;

#define preDebug 0

// interacting between frontend and backend.
preprocessor::preprocessor(Parser* p, Collector* c, model_s* m):
    parser(p), thector(c), model(m)
{
    // init
    data = &(p->dag);
    model->setVariables(data->vbool_list);
    model->setVariables(data->vnum_int_list);
    state = State::UNKNOWN;
    nnfer = new nnf_rewriter(p);
    logicer = new logic_rewriter(p);
    eqer = new eq_rewriter(p);
    eqer->setModel(m);
    polyer = new poly_rewriter(p);
    comper = new comp_rewriter(p, polyer, eqer);
    proper = new prop_rewriter(p, m);
    proper->setNNF(nnfer);
    leter = new let_rewriter(p, m);
    eqsver = new eq_solver(p, m);
    eqsver->setRewriters(eqer, polyer);
    eqsver->setCollector(thector);
    message = new Message();
    message->data = data;
    message->model = model;
}
preprocessor::~preprocessor(){
    delete message; message = nullptr;
    delete proper;  proper = nullptr;
    delete comper;  comper = nullptr;
    delete polyer;  polyer = nullptr;
    delete eqer;    eqer = nullptr;
    delete logicer; logicer = nullptr;
    delete nnfer;   nnfer = nullptr;
    delete leter;   leter = nullptr;
    delete eqsver;  eqsver = nullptr;
}

void preprocessor::auto_set_model(){
    // use collector to set model
    // all to intervals
    message->state = State::SAT;
    std::vector<dagc*> vars;
    message->model->getUnassignedVars(vars);
    for(size_t i=0;i<vars.size();i++){
        feasible_set* fs = thector->variables[vars[i]];
        assert(fs != nullptr);
        Interval intv = fs->choose();
        poly::Integer val = to_Integer(poly::pick_value(intv));
        message->model->set(vars[i], to_long(val));
    }
}

// simplify funtions

bool preprocessor::check(bool er){
    if(er) return true;
    else{
        message->state = State::UNSAT;
        return false;
    }
}
bool preprocessor::simplify(){
    simplified = true;
    #if preDebug
        std::cout<<"\noriginal formulas..."<<std::endl;
        data->print_constraints();
        // 0. convert to nnf-like tree.
        std::cout<<"\nnnf rewriting..."<<std::endl;
    #endif
    nnfer->rewrite();

    #if preDebug
        data->print_constraints();
        // 1. eliminate top(/true) let
        std::cout<<"\nlet operator rewriting..."<<std::endl;
    #endif
    if(!check(leter->rewrite())) return false;

    #if preDebug
        data->print_constraints();
        // 2. eliminate logic operators.
        std::cout<<"\nlogic operation rewriting..."<<std::endl;
    #endif
    logicer->rewrite();
    #if preDebug
        data->print_constraints();
        // 3. comp rewriting.
        //  1) add t := v to model in equivalence rewriter,
        //  2) add x := y to model in equivalence rewriter.
        std::cout<<"\ncomparision equal rewriting..."<<std::endl;
    #endif
    if(!check(eqer->rewrite())) return false;
    
    // data->print_smtlib();
    #if preDebug
        data->print_constraints();
        //  3) comp rewriting,
        std::cout<<"\ncomparision comp rewriting..."<<std::endl;
    #endif
    if(!check(comper->rewrite())) return false;
    // data->print_constraints();
    // model->print_partial();
    // data->print_smtlib();

    #if preDebug
        data->print_constraints();
        // 4. model propagation rewriting.
        std::cout<<"\npropagation rewriting..."<<std::endl;
    #endif
    if(!check(proper->rewrite())) return false;

    // model->print_partial();
    // data->print_smtlib();

    // remove true
    #if preDebug
        data->print_constraints();
        std::cout<<"\nremoving sat constraints..."<<std::endl;
    #endif
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->istrue()){
            (*it)->print();
            it = data->assert_list.erase(it);
        }
        else if((*it)->isfalse()){
            (*it)->print();
            message->state = State::UNSAT;
            return false;
        }
        else ++it;
    }
    // data->print_smtlib();
    // model->print_partial();
    if(data->assert_list.size() == 0){
        state = State::SAT;
        return true;
    }
    
    #if preDebug
        data->print_constraints();
        std::cout<<"\ncollector rewriting..."<<std::endl;
    #endif

    // new appended 05/28
    nnfer->rewrite();
    // 5. collector
    if(!check(thector->rewrite())) return false;
    // new appended 06/02
    // x in [0, 1) -> x = 0
    #if preDebug
        data->print_constraints();
        std::cout<<"\nre-propagte rewriting..."<<std::endl;
    #endif

    // model->print_partial();
    if(!check(proper->rewrite())) return false;
    // model->print_partial();
    
    // thector->print_feasible_set();
    // data->print_smtlib();
    #if preDebug
        data->print_constraints();
    #endif


    if(data->assert_list.size() == 0){
        // all eliminated
        // auto set model
        auto_set_model();
        state = State::SAT;
    }
    // data->print_constraints();
    // data->print_smtlib();

    // eq solver
    // now only use rewrite
    if(!check(eqsver->solve())) return false;

    return true;
}


Message* preprocessor::release(){
    // unsat
    if(message->state==State::UNSAT) return nullptr;
    // sat -> model
    if(message->state==State::SAT) return message;
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        message->constraints.emplace_back(*it);
        ++it;
    }
    // unknown
    message->state = State::UNKNOWN;
    return message;
}