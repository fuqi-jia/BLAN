/* qfnia.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "qfnia/qfnia.hpp"
#include "qfnia/checker.hpp"
using namespace ismt;

#define doCheck 0

qfnia_solver::qfnia_solver(){}
qfnia_solver::~qfnia_solver(){
    if(used){
        delete data;        data = nullptr;
        delete prep;        prep = nullptr;
        delete collector;   collector = nullptr;
        delete decider;     decider = nullptr;
        delete resolver;    resolver = nullptr;
        delete searcher;    searcher = nullptr;
    }
}

void qfnia_solver::init(Info* info){
    decider = new Decider();
    searcher = new Searcher();
    resolver = new Resolver();
    decider->init(info);
    searcher->init(info);
    resolver->init(info);
}

// main function
int qfnia_solver::solve(SolverOptions* option){
    std::string file = option->File;
    used = true;
    data = new DAG();

    benchmark = file;
    model_s* model = new model_s();

    // parse the file
    // std::cout<<"new parser start"<<std::endl;
    Parser parser(file, *data);
    // std::cout<<"new parser end"<<std::endl;
    collector = new Collector(&parser, model);

    // std::cout<<"new preprocessor start"<<std::endl;
    prep = new preprocessor(&parser, collector, model);
    // std::cout<<"new preprocessor end"<<std::endl;
    bool ans = prep->simplify();
    if(ans){
        if(prep->state == State::SAT){
            std::cout<<"sat"<<std::endl;
            if(data->get_model){
                Message* message = prep->release();
                message->print_model();
            }
            return 0;
        }
    }
    else{
        std::cout<<"unsat"<<std::endl;
        return 0;
    }
    // std::cout<<"simplify done"<<std::endl;
    Message* message = prep->release();
    // model->print();
    // message->print_constraints();

    // start to solve
    Info* info = new Info(message, collector, prep, option);
    init(info);
    // model->print_partial();

    // main loop
    while(true){
        while(decider->decide()){}
        if(decider->conflict()){
            // decide with bit blasting overflow
            // std::cout<<"unknown"<<std::endl;
            // invoking yices2
            // exit(0);
            break;
        }
        if(!searcher->search()){
            if(!resolver->resolve()){
                state = State::UNSAT;
                break;
            }
            else{
                if(resolver->has_lemma()) message->constraints.insert(message->constraints.begin(), resolver->lemma()); // add lemma in the front
                // must increment
                decider->increment();
                searcher->reset();
            }
        }
        else{
            if(decider->done()) break;
        }
    }

    // int bound = 100;
    // int cnt = 0;
    // while(true){
    //     if(decider->decide()){
    //         ++cnt;
    //     }
    //     if(decider->conflict()){
    //         // decide with bit blasting overflow
    //         std::cout<<"to do, conflict"<<std::endl;
    //         // invoking yices2
    //         exit(0);
    //     }
    //     if(cnt >= bound){
    //         if(!searcher->search()){
    //             if(!resolver->resolve()){
    //                 state = State::UNSAT;
    //                 break;
    //             }
    //             else{
    //                 if(resolver->has_lemma()) message->constraints.insert(message->constraints.begin(), resolver->lemma()); // add lemma in the front
    //                 // must increment
    //                 decider->increment();
    //                 searcher->reset();
    //             }
    //         }
    //         else{
    //             if(decider->done()) break;
    //         }
    //         cnt = 0;
    //     }
    // }


    if(info->state==State::SAT){
        searcher->set_model();
        std::cout<<"sat"<<std::endl;
        if(data->get_model) message->print_model();
        #if doCheck
            checker ch(info);
            if(ch.check()){
                std::cout<<"check sat"<<std::endl;
            }
            else{
                std::cout<<"check unsat"<<std::endl;
            }
        #endif
    }
    else if(info->state==State::UNKNOWN){
        std::cout<<"unknown"<<std::endl;
    }
    else{
        std::cout<<"unsat"<<std::endl;
    }

    return 0;
}