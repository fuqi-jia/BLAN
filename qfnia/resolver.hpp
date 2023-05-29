/* resolver.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _RESOLVER_H
#define _RESOLVER_H

#include "qfnia/info.hpp"

namespace ismt
{
    class Resolver
    {
    private:
        Info* info = nullptr;
        Parser* parser = nullptr;
        poly_rewriter* polyer = nullptr;
        nnf_rewriter* nnfer   = nullptr;
        std::vector<dagc*> core;
        
        bool is_hard(std::vector<dagc*>& constraints);
        bool is_hard(dagc* constraint);
        bool cad_resolve(std::vector<dagc*>& constraints);
    public:
        Resolver(/* args */);
        ~Resolver();

        void init(Info* i);
        // mcsat use different sovlers
        bool resolve();
        dagc* lemma();
        bool has_lemma() const;
    };
    
} // namespace ismt


#endif