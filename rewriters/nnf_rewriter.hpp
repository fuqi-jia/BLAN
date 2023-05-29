/* nnf_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _NNF_REWRITER_H
#define _NNF_REWRITER_H

#include "rewriters/rewriter.hpp"

namespace ismt
{
    class nnf_rewriter: public rewriter
    {
    private:
        // convert to nnf-like tree
        void toNNFNode(dagc*& node, bool isNot);         // convert the node
        bool isVar(dagc* node);
    public:
        nnf_rewriter(Parser* p): rewriter(p) {}
        ~nnf_rewriter(){}

        void rewrite(dagc*& root, bool isNot = false);
        void rewrite();
    };
    
} // namespace ismt


#endif
