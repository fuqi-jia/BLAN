/* cdcl_t_solver.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _CDCL_T_SOLVER_H
#define _CDCL_T_SOLVER_H

#include "solvers/sat/sat_solver.hpp"
#include "utils/constraint.hpp"

namespace ismt
{
    class cdcl_t_solver
    {
    private:
        sat_solver* decider = nullptr;
        size_t lit_size = 0;
    public:
        cdcl_t_solver(/* args */);
        ~cdcl_t_solver();

        bool decide();
        bool bcp();
        int new_lit(constraint& lit);
        void add_clause(vector<int>& c);
        void current_assignment(std::vector<int>& partial_model);
    };
} // namespace ismt


#endif