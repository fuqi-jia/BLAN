/* rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _REWRITER_H
#define _REWRITER_H

#include "frontend/dag.hpp"
#include "frontend/parser.hpp"

namespace ismt
{
    class rewriter
    {
    protected:
        DAG* data = nullptr;
        Parser* parser = nullptr;
    public:
        bool consistent = true;
        
        rewriter(Parser* p):parser(p){
            data = &(p->dag);
        }
        ~rewriter(){}
    };
    
    
} // namespace ismt


#endif