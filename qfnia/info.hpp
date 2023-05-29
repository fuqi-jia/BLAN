/* info.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _INFO_H
#define _INFO_H

#include "midend/message.hpp"
#include "midend/preprocessor.hpp"
#include "qfnia/collector.hpp"
// 2023-05-27
#include "options.hpp"
#include <boost/unordered_map.hpp>

namespace ismt
{
    struct Info
    {
        State state = State::UNKNOWN;
        Message* message = nullptr;
        Collector* collector = nullptr;
        preprocessor* prep = nullptr;
        // 2023-05-27
        SolverOptions* options = nullptr;
        boost::unordered_map<dagc*, Interval> assignment;
        std::vector<dagc*> variables;
        std::vector<dagc*> constraints;
        std::vector<int> assumptions;
        std::vector<int> constraint_levels;
        std::vector<int> assumption_levels;
        std::vector<dagc*> new_constraints;
        std::vector<dagc*> new_vars;

        // decide -> search -> unsat -> resolve -> decide
        //                  -> sat
        //                  -> unknown -> decide
        //        -> sat
        bool resolve = false; // once resolve, reset the constraints and assignments 

        Info(Message* m, Collector* c, preprocessor* pr, SolverOptions* opt): message(m), collector(c), prep(pr), options(opt){}
        ~Info(){}

        void print_constraints(){
            for(size_t i=0;i<constraints.size();i++){
                std::cout<<assumptions[i] <<": ";
                message->data->printAST(constraints[i]);
                std::cout<<std::endl;
            }
        }

        void reset(){
            constraints.clear();
            assumptions.clear();
            variables.clear();
            assignment.clear();
        }
        // void add_variables(dagc* var){
        //     variables.emplace_back(var);
        // }
        // void add_assumptions(vector<int>& assumps){
        //     for(unsigned i=0;i<assumps.size();i++){
        //     }
        // }
        // void add_constraints(){
        // }
        // void backtrack(){
        // }
    };
    
} // namespace ismt


#endif