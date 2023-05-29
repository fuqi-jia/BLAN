/* op_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _OP_REWRITER_H
#define _OP_REWRITER_H

#include "frontend/dag.hpp"
#include "utils/model.hpp"
#include "utils/disjoint_set.hpp"
#include "rewriters/rewriter.hpp"

namespace ismt
{
    // currently only rewrite abs
    // not support, inside polynomial -> explosion
    // abs(x+y) > 3 -> -(x + y) > 3 or (x+y) > 3
    // abs(x+y) > 0 -> x+y != 0
    // abs(x+y) >= 0 -> true
    // x + abs(x+y) = 3 -> x + (x+y) = 3 or x - (x+y) = 3
    // x + abs(x) + abs(y) + abs(z) = 3 -> 8, ... so not support
    // abs(abs(...(x+y))) -> abs(x+y) assumes never
    class op_rewriter: public rewriter
    {
    public:
        op_rewriter(Parser* p): rewriter(p) {}
        ~op_rewriter(){}

        dagc* rewrite(dagc* root);
    };

} // namespace ismt


#endif
