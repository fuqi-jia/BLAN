/* let_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _LET_REWRITER_H
#define _LET_REWRITER_H

#include "frontend/dag.hpp"
#include "rewriters/rewriter.hpp"
#include "utils/model.hpp"

namespace ismt
{
    class let_rewriter: public rewriter
    {
    private:
        model_s* model;
        dagc* content(dagc* root); // content of letvar
        void analyzeOp(dagc* root); // or / eqbool / neqbool
        void rewrite(dagc* root, std::vector<dagc*>& res, bool isTop = true);
        bool propagate(dagc* root, std::vector<dagc*>& res);
    public:
        let_rewriter(Parser* p, model_s* m): rewriter(p), model(m) {
            data = &(p->dag);
        }
        ~let_rewriter(){}

        bool rewrite();
    };

} // namespace ismt


#endif
