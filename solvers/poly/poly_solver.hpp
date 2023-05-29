/* poly_solver.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _POLY_SOLVER_H
#define _POLY_SOLVER_H

#include "rewriters/poly_rewriter.hpp"

namespace ismt
{
    // an implementation of nisat ( non-linear integer mcsat solver )
    class poly_solver
    {
    private:
        poly_rewriter* polyer = nullptr;
        poly::Assignment* data = nullptr;
    public:
        poly_solver(/* args */){}
        ~poly_solver(){}

        // solving functions
        State check();
        // unsat core if failed -> lemma
        dagc* core();



        // // add operations
        // void assert(dagc* constraint);
        // void assign(dagc* var, const poly::Integer& v);
        // void assign(dagc* var, const Integer& v);
    };
    
} // namespace ismt


#endif