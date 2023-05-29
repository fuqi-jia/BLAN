/* preprocessor.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _PREPROCESSOR_H
#define _PREPROCESSOR_H

#include "frontend/dag.hpp"
#include "frontend/parser.hpp"
#include "utils/disjoint_set.hpp"
#include "utils/feasible_set.hpp"

#include "rewriters/nnf_rewriter.hpp"
#include "rewriters/logic_rewriter.hpp"
#include "rewriters/eq_rewriter.hpp"
#include "rewriters/poly_rewriter.hpp"
#include "rewriters/comp_rewriter.hpp"
#include "rewriters/prop_rewriter.hpp"
#include "rewriters/let_rewriter.hpp"

#include "midend/message.hpp"
#include "qfnia/collector.hpp"
#include "solvers/eq/eq_solver.hpp"

namespace ismt
{   
    // preprocess dag and store informations. 
    class preprocessor
    {
    public: // now public
        // inner objects
        State   state;

        // frontend objects
        DAG*    data = nullptr;
        Parser* parser = nullptr;

        // rewriters
        nnf_rewriter* nnfer = nullptr;
        logic_rewriter* logicer = nullptr;
        eq_rewriter* eqer = nullptr;
        poly_rewriter* polyer = nullptr;
        comp_rewriter* comper = nullptr;
        prop_rewriter* proper = nullptr;
        let_rewriter* leter = nullptr;

        // backend theory collector
        Collector* thector = nullptr;

        // simple equal closure solver
        eq_solver* eqsver = nullptr;

        // interacting between frontend and backend.
        Message* message;
        bool simplified = false; // whether simplified.
        model_s* model = nullptr;

        void auto_set_model ();
        bool check(bool er);
    public:
        preprocessor(Parser* p, Collector* c, model_s* m);
        ~preprocessor();

        // simplify: 
        // 1. convert to nnf-like tree.
        // 2. eliminate logic operators.
        // 3. comp rewriting.
        // 4. model propagation rewriting.
        // 5. collect must, may bound and neq nodes with counting used variables.
        // 6. simplify unconstrainted variables.
        // 7. analyze each comp constraint and check bounds.
        // +8. leave a seat vacant for new idea.
        bool simplify();
        void printAst();

        Message* release(); // release message.
    };
    
} // namespace ismt


#endif