/* icp_solver.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _ICP_SOLVER_H
#define _ICP_SOLVER_H

#include "utils/types.hpp"
#include "utils/interval.hpp"
#include "rewriters/poly_rewriter.hpp"
#include "solvers/sat/sat_solver.hpp"

namespace ismt
{
    class icp_solver
    {
    private:
        poly_rewriter* polyer;
        poly::IntervalAssignment* data;
        sat_solver* controller; // top sat solver

        std::vector<dagc*> stored_core;
        bool isEasyComp(dagc* c); 
        bool easyComp(dagc* c); 

    public:
        icp_solver(poly_rewriter* p);
        ~icp_solver();

        // solving functions
        bool evaluate(dagc* c); 
        State check(std::vector<dagc*>& cs);
        // unsat core if failed -> lemma
        void core(std::vector<dagc*>& cores);
        
        // add operations
        void assign(dagc* var, const poly::Interval& v);
        void reset();
    };
    
} // namespace ismt


#endif