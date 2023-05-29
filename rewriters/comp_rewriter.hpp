/* comp_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _COMP_REWRITER_H
#define _COMP_REWRITER_H

#include "utils/poly.hpp"
#include "utils/interval.hpp"
#include "rewriters/rewriter.hpp"
#include "rewriters/eq_rewriter.hpp"
#include "rewriters/poly_rewriter.hpp"

namespace ismt
{
    // equal:
    // 1) no-add-mul:
    //  x = 3 -> x = 3
    // 2) only add:
    //  x + 1 = 3 -> x = 2
    // 3) only mul:
    //  3*x = 3 -> x = 1
    //  3*x = 4 -> false
    //  3*x*y = 0 -> x = 0 \/ y = 0 
    //  3*x*y = 3 -> x*y = 1  // (not implement) -> (x = 1 /\ y = 1) \/ (x = -1 /\ y = -1)
    // 4) add-mul: (c!=0, d in (-oo, +oo))
    //  3*x + 1 = 4 -> 3*x = 3 -> x = 1, 3*x + 1 = 5 -> 3*x = 4 -> false
    //  3*(x + 1) = 3 -> x + 1 = 1 -> x = 0
    //  x*y + (-1)*x*y = 0 -> 0 = 0 -> true.
    //  x*y + x*y = 0 -> 2*x*y = 0 -> x*y = 0 -> x = 0 \/ y = 0
    //  x*y + y*z = 0 -> x*y = - y*z -> y = 0 \/ x = -z
    //  x*y + (-1)*y*z = 0 -> x*y = y*z -> y = 0 \/ x = z
    //  x*y + (-1)*x*y = c -> 0 = c -> false.
    //  x*y + x*y = c -> 2*x*y = c -> x*y = c // 2
    //  x*y + y*z = c -> x*y + y*z = c
    //  u*v + x*y + y*z = d -> u*v + x*y + y*z = d
    //  u*v + x*y + (-1)*y*z = d -> u*v + x*y = y*z +d (minus -> transposition)

    // neq:
    // 1) no-add-mul:
    //  x != 3 -> x != 3
    // 2) only add:
    //  x + 1 != 3 -> x != 2
    // 3) only mul:
    //  3*x = 3 -> x = 1
    //  3*x = 4 -> false
    //  3*x*y = 0 -> x = 0 \/ y = 0 
    //  3*x*y = 3 -> x*y = 1  // (not implement) -> (x = 1 /\ y = 1) \/ (x = -1 /\ y = -1)
    // 4) add-mul: (c!=0, d in (-oo, +oo))
    //  3*x + 1 = 4 -> 3*x = 3 -> x = 1, 3*x + 1 = 5 -> 3*x = 4 -> false
    //  3*(x + 1) = 3 -> x + 1 = 1 -> x = 0
    //  x*y + (-1)*x*y = 0 -> 0 = 0 -> true.
    //  x*y + x*y = 0 -> 2*x*y = 0 -> x*y = 0 -> x = 0 \/ y = 0
    //  x*y + y*z = 0 -> x*y = - y*z -> y = 0 \/ x = -z
    //  x*y + (-1)*y*z = 0 -> x*y = y*z -> y = 0 \/ x = z
    //  x*y + (-1)*x*y = c -> 0 = c -> false.
    //  x*y + x*y = c -> 2*x*y = c -> x*y = c // 2
    //  x*y + y*z = c -> x*y + y*z = c
    //  u*v + x*y + y*z = d -> u*v + x*y + y*z = d
    //  u*v + x*y + (-1)*y*z = d -> u*v + x*y = y*z +d (minus -> transposition)


    // x > 3 -> x > 3
    // x + 1 > 3 -> x > 2 
    // 3*x*y == 9 -> x*y = 3
    // 3*x*y == 0 -> x = 0 \/ y = 0 
    // x + y + 3 > 4 -> x + y > 1
    // 3*x*y + 3 = 3 -> 3*x*y = 0 -> x = 0 \/ y = 0
    // 12 + (-1)*x < 0 -> x > 12
    // 12 + (-1)*x + (-1)*y < 0 -> x + y > 12
    
    class comp_rewriter: public rewriter
    {
    private:
        bool curIsTop = false;
        // bool integer = true;
        poly::Context context;
        
        poly_rewriter*  polyer = nullptr;
        eq_rewriter*    eqer = nullptr;

    private:
        // void transposit(poly::Polynomial& p, poly::Polynomial& ls, poly::Polynomial& rs);
        dagc* eq_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs);
        dagc* le_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs);
        dagc* lt_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs);
        dagc* ge_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs);
        dagc* gt_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs);
        dagc* neq_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs);

        // helper function
        dagc* univariate_eq(poly::Polynomial& p);
        dagc* univariate_comp(poly::Polynomial& p, bool g, bool e);

        void roots_isolate_int(poly::Polynomial& p, std::vector<Value>& rets);
        void roots_isolate_comp_int(poly::Polynomial& p, bool g, bool e, std::vector<Interval>& rets);
 
        void roots_isolate(poly::Polynomial& p, std::vector<Value>& rets);
        void roots_isolate_compt(poly::Polynomial& p, bool g, bool e, std::vector<Interval>& rets);
        
        bool evaluate(poly::UPolynomial& up, bool g, bool e, poly::Integer& val);

        bool isSimpleComp(dagc* root);
        bool isNormForm(dagc* root);
        // x*y*z = 0 -> x = 0 \/ y = 0 \/ z = 0.
        dagc* eqZero(dagc* var);
        dagc* orEqZero(std::vector<dagc*> vars);
        dagc* andEqZero(std::vector<dagc*> vars);
        dagc* rewrite(dagc* root, bool isTop = true);
        
    public:
        comp_rewriter(Parser* p, poly_rewriter* po, eq_rewriter* eq);
        ~comp_rewriter();
        
        // make name
        bool rewrite();
    };
    
} // namespace ismt


#endif