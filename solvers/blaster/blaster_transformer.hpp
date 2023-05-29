/* blaster_transformer.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _BLASTER_TRANSFORMER_H
#define _BLASTER_TRANSFORMER_H

#include "frontend/dag.hpp"
#include "solvers/blaster/blaster_solver.hpp"

// 2023-05-27
#include "options.hpp"

namespace ismt
{   
    // helper for transforming dag formula to bv.
    // it is a default transformer. blaster_solver has mkHolder/blast...
    class blaster_transformer
    {
    private:
        /* data */
        blaster_solver* solver;
        // bool try_split = true;
        
        // map dagc* to inner representation.
        boost::unordered_map<dagc*, int>    BoolMap;
        boost::unordered_map<dagc*, int>    BoolLetMap;
        boost::unordered_map<dagc*, int>    BoolFunMap;
        boost::unordered_map<dagc*, int>    BoolOprMap;
        boost::unordered_map<dagc*, bvar>   VarMap;
        boost::unordered_map<dagc*, bvar>   IntMap;
        boost::unordered_map<dagc*, bvar>   OprMap;
        boost::unordered_map<dagc*, bvar>   NumLetMap;
        boost::unordered_map<dagc*, bvar>   IntFunMap;

        // declare a int to blaster
        void declareInt(dagc* root);
        bvar getInt(dagc* root);
        bvar getLetNum(dagc* root);
        int getLetBool(dagc* root);
        int getBool(dagc* root);

        // declare a bool to blaster
        void declareBool(dagc* root);

        // transform
        int         doAtoms(dagc* dag);
        int         doMathAtoms(dagc* dag);
        int         doLetAtoms(dagc* dag);
        bvar        doIteNums(dagc* dag);
        bvar        doMathTerms(dagc* dag);
        bvar        doLetNum(dagc* dag);
        bool        isFree(Integer lower, bool a_open, Integer upper, bool b_open);
        int         split(Integer lower, bool a_open, Integer upper, bool b_open);
        int         split_lower(Integer lower, bool a_open, Integer upper, bool b_open);
        int         split_middle(Integer lower, bool a_open, Integer upper, bool b_open);
        void        _declare(dagc* root, unsigned bit);

    public:
        blaster_transformer(blaster_solver* s){ solver = s; }
        ~blaster_transformer(){}

        // 2023-05-27
        bool        GA = true;

        // transform a constraint to blaster
        int transform(dagc* root);

        // declare a var to blaster
        void _declare(dagc* root, Integer lower, bool a_open, Integer upper, bool b_open);
        void _declare(dagc* root);
        int  _assert(dagc* root);
        
        // clear variables
        void reset(blaster_solver* s);
        void reset();
    };

    
} // namespace ismt

#endif