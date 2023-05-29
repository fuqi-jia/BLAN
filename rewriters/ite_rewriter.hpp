/* ite_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _ITE_REWRITER_H
#define _ITE_REWRITER_H

#include "frontend/dag.hpp"
#include "rewriters/rewriter.hpp"
#include "utils/model.hpp"

namespace ismt
{
    class ite_rewriter: public rewriter
    {
    private:
        model_s* model;
        void rewrite(dagc* root, bool isTop, std::vector<dagc*>& res);
        bool analyze(dagc* root, std::vector<dagc*>& res, bool isnot = false);
    public:
        ite_rewriter(Parser* p, model_s* m): rewriter(p), model(m) {
            data = &(p->dag);
        }
        ~ite_rewriter(){}

        bool rewrite();
    };

} // namespace ismt


#endif
