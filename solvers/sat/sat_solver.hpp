/* sat_solver.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _SAT_SOLVER_H
#define _SAT_SOLVER_H

#include "utils/types.hpp"
#include "frontend/dag.hpp"
#include "solvers/sat/sat_types.hpp"

// only cadical
#include "solvers/include/cadical.hpp"


#include<string>
#include<vector>

namespace ismt
{
    class sat_solver
    {
    private:
        CaDiCaL::Solver*    solver = nullptr;

        std::string         benchmark;
        std::string         name;

        // solver state
        State               state;

        // variable index
        unsigned            iVars;
        unsigned            iClauses;
        int                 SAT_False;
        int                 SAT_True;

        // cdcl(t)
        literals            assumptions;

        // inner functions
        void setConst();
    public:
        sat_solver(std::string n = "cadical");
        ~sat_solver();

        // get false
        bool_var    False() const;
        bool_var    True()  const;

        // number of vars
        int         nVars() const;
        int         nClauses() const;


        // idx of current variables
        bool_var    curVar() const;

        // add to solver
        int         newVar();
        int         newVars(unsigned num);
        bool        addClause(clause& c);
        
        // solve
        bool        simplify(const literals& c); // means simple solve
        bool        solve();
        bool        solve(const literals& aps);

        // state
        State       getState() const;

        // model
        model       getModel();

        // reset
        void        reset();

        // benchmark setting
        void        setBenchmark(std::string file);
        
        // cdcl(t) function
        bool        propagate();
        void        partial(boost::unordered_map<int, int>& m);
        int         val(int lit);

        // get unsat assumptions
        void        getConflict(literals& aps);
    };
    
} // namespace ismt


#endif