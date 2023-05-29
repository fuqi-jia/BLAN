/* prop_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _PROP_REWRITER_H
#define _PROP_REWRITER_H

#include "utils/model.hpp"
#include "rewriters/nnf_rewriter.hpp"
#include "rewriters/eq_rewriter.hpp"
#include <vector>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

namespace ismt
{
    // TODO, currently ite is not support
    class prop_rewriter: public rewriter
    {
    private:
        // rewriter
        nnf_rewriter* nnfer = nullptr;
        model_s* model;
        State state = State::UNKNOWN ;
        std::vector<dagc*> todo; // new assigned variables to propagate
        boost::unordered_set<dagc*> currents; // current propagating variables
        // boost::unordered_set<dagc*> top_comp; // top comp constaints, so default sat, if not it then false
        void rewriteComp(dagc* root, bool isTop, std::vector<dagc*>& res);
        bool evaluate(dagc* root, Integer& l, Integer& r);
        bool single(dagc*& rec, dagc* root, int& len);
        void convert(dagc* root, bool isTop, dagc* var, std::vector<dagc*>& res);
        void convertAbs(dagc* root, std::vector<dagc*>& res);
        // TODO these 3 important.
        void propagateTrue(dagc* root, std::vector<dagc*>& res);  // the atom is true and propagate
        void propagateFalse(dagc* root, std::vector<dagc*>& res); // the atom is false and propagate
        void propagatePart(dagc* root, bool isTrue, std::vector<dagc*>& res); // the atom is isTrue and propagate
        void eval_rewrite(dagc*& root); // evaluate the atom via model and rewrite root
        void evaluatePoly(dagc*& root);
        // (= a b) -> false
        // if a -> false => not b -> false => b -> true
        // if a -> true  => b -> false     => b -> false
        // (= a c) -> false c = (...)
        // if a -> false => not c -> false
        // if a -> true  => c -> true

        // down propagate the assigned value, up save the real value. 
    public:
        prop_rewriter(Parser* p, model_s* m): rewriter(p), model(m) {
            data = &(p->dag);
        }
        ~prop_rewriter(){}
        void setNNF(nnf_rewriter* nr);
        void setTodo();
        bool rewrite();
    };
    
} // namespace ismt


#endif
