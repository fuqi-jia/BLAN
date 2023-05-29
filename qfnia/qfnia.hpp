/* qfnia.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _QFNIA_H
#define _QFNIA_H

// frontend
#include "frontend/parser.hpp"
// midend
#include "midend/preprocessor.hpp"
#include "midend/message.hpp"
// backend
#include "qfnia/info.hpp"
#include "qfnia/collector.hpp"
#include "qfnia/decider.hpp"
#include "qfnia/resolver.hpp"
#include "qfnia/searcher.hpp"

// 2023-05-27
#include "options.hpp"

namespace ismt
{
    // static poly::Context context();
    const int maxVars = 256;
    class qfnia_solver{
    private:
        DAG* data;
        bool used = false;
        std::string benchmark;
        State state = State::UNKNOWN;
        // midend
        preprocessor* prep = nullptr;
        // qfnia part
        Collector* collector = nullptr;
        Decider* decider = nullptr;
        Resolver* resolver = nullptr;
        Searcher* searcher = nullptr;
    public:
        qfnia_solver();
        ~qfnia_solver();
        void printModel();
        // main function
        void init(Info* info);
        // int solve(const std::string& file);
        int solve(SolverOptions* option);
    };
} // namespace ismt


#endif

// Decide interval for variables, 
//  if sat under ICP, then give constraints to blaster to solve, if sat then we get a model.
//  once unsat, try to resolve it via nlsat procedure and generate a lemma.