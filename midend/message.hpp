/* message.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "frontend/dag.hpp"
#include "utils/feasible_set.hpp"
#include "utils/types.hpp"
#include "utils/model.hpp"
#include <vector>
#include <list>
#include <boost/unordered_map.hpp>

namespace ismt
{
    // message from preprocessor
    struct Message{
        // 0. DAG
        DAG * data;
        // 1. constraints: after preprocess, they may changed.
        std::list<dagc*> constraints;
        // 2. midend state
        State state = State::UNKNOWN;
        // 3. preprocessed model, and variables
        model_s* model;

        void print_constraints(){
            auto it = constraints.begin();
            while(it != constraints.end()){
                data->printAST(*it);
                std::cout<<std::endl;
                ++it;
            }
        }
        void print_model(){
            model->print();
        }
    };
} // namespace ismt


#endif