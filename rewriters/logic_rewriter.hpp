/* logic_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _LOGIC_REWRITER_H
#define _LOGIC_REWRITER_H

#include "frontend/dag.hpp"
#include "rewriters/rewriter.hpp"

namespace ismt
{
    // split each and-constraint
    // 1) assert-and -> asserts, (assert (and a b c)) -> (assert a) (assert b) (assert c).
    // 2) and-and -> and
    // 3) or-or -> or
    // 4) and-or/or-and -> and->or/or-and
    class logic_rewriter: public rewriter
    {
    private:
        bool curIsTop = false;

        void andRecord(dagc* root, std::vector<dagc*>& res);
        void orRecord(dagc* root, std::vector<dagc*>& res);
        void rewrite(dagc* root);
    public:
        logic_rewriter(Parser* p): rewriter(p) {}
        ~logic_rewriter(){}
        
        void rewrite();
    };

} // namespace ismt


#endif
