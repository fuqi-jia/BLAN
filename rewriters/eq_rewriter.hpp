/* eq_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _EQ_REWRITER_H
#define _EQ_REWRITER_H

#include "frontend/dag.hpp"
#include "utils/model.hpp"
#include "utils/disjoint_set.hpp"
#include "rewriters/rewriter.hpp"

namespace ismt
{
    // inner each and, we can use equivalence_rewriter to locally propagate
    class eq_rewriter: public rewriter
    {
    private:
        bool curIsTop = false;
        model_s* model = nullptr;

        bool localAssign(boost::unordered_map<dagc*, Integer>& localAssigned, dagc* root, Integer val);
        bool localSubsitute(disjoint_set<dagc*>& localSubset, dagc* a, dagc* b);
    public:
        eq_rewriter(Parser* p): rewriter(p) {}
        ~eq_rewriter(){}

        void setModel(model_s* m);
        dagc* rewrite(dagc* root, bool isTop = true);
        dagc* localRewrite(dagc* root, disjoint_set<dagc*>& localSubset, boost::unordered_map<dagc*, Integer>& localAssigned);

        bool rewrite();
    };

} // namespace ismt


#endif
