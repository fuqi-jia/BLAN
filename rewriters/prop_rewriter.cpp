/* prop_rewriter.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "rewriters/prop_rewriter.hpp"
#include <stack>

using namespace ismt;

void prop_rewriter::evaluatePoly(dagc*& root){
    if(root->isAssigned()){ // as well as root->isvnum()
        root = parser->mk_const(root->v);
    }
    else if(root->isnumop()){
        if(root->isabs()){
            evaluatePoly(root->children[0]);
            dagc* x = root->children[0];
            if(x->isAssigned()){
                Integer res = (x->v>=0?x->v:-x->v); // abs
                root->assign(res);
                root = parser->mk_const(root->v);
            }
        }
        else if(root->isletnum()){
            evaluatePoly(root->children[0]);
            dagc* x = root->children[0];
            if(x->isAssigned()){
                root->assign(x->v);
                root = parser->mk_const(root->v);
            }
        }
        else if(root->isadd()){
            Integer val = 0;
            bool all_assigned = true;
            for(size_t i=0;i<root->children.size();i++){
                evaluatePoly(root->children[i]);
                dagc* x = root->children[i];
                if(x->isAssigned()){
                    val += x->v;
                }
                else{
                    all_assigned = false;
                }
            }
            if(all_assigned){
                root->assign(val);
                root = parser->mk_const(val);
            }
        }
        else if(root->issub()){
            // sub -> add, if sub then assert(false)
            assert(false);
        }
        else if(root->ismul()){
            Integer val = 1;
            bool all_assigned = true;
            for(size_t i=0;i<root->children.size();i++){
                evaluatePoly(root->children[i]);
                dagc* x = root->children[i];
                if(x->isAssigned()){
                    val *= x->v;
                }
                else{
                    all_assigned = false;
                }
            }
            if(all_assigned){
                root->assign(val);
                root = parser->mk_const(val);
            }
            else if(val == 0){
                // val *= 0 -> val = 0.
                root->assign(0);
                root = parser->mk_const(0);
            }
        }
        else if(root->isdiv()){
            // y / x
            evaluatePoly(root->children[0]);
            evaluatePoly(root->children[1]);
            dagc* y = root->children[0];
            dagc* x = root->children[1];
            if(y->isAssigned() && x->isAssigned()){
                if(y->v % x->v != 0){
                    assert(false);
                }
                else{
                    Integer res = y->v / x->v;
                    root->assign(res);
                    root = parser->mk_const(res);
                }
            }
        }
        else if(root->ismod()){
            // y % x
            evaluatePoly(root->children[0]);
            evaluatePoly(root->children[1]);
            dagc* y = root->children[0];
            dagc* x = root->children[1];
            assert(x->iscnum());
            if(y->isAssigned() && x->isAssigned()){
                Integer res = y->v % x->v;
                root->assign(res);
                root = parser->mk_const(res);
            }
        }
        else assert(false);
    }
}

bool prop_rewriter::evaluate(dagc* root, Integer& l, Integer& r){
    if(root->iseq()) return l==r;
    else if(root->isneq()) return l!=r;
    else if(root->isle()) return l<=r;
    else if(root->islt()) return l<r;
    else if(root->isge()) return l>=r;
    else if(root->isgt()) return l>r;
    else assert(false);
}
bool prop_rewriter::single(dagc*& rec, dagc* root, int& len){
    if(root->ismod()) return false; // 2023-05-18: mod cannot be simplified.
    for(size_t i=0;i<root->children.size();i++){
        dagc* x = root->children[i];
        if(!x->isAssigned()){
            if(x->isvnum()){
                rec = x;
                ++len;
            }
            else{
                if(!single(rec, x, len)) return false;
            }
            if(len > 1) return false;
        }
    }

    if(rec == nullptr || len > 1) return false;
    else return true;
}

void prop_rewriter::convertAbs(dagc* root, std::vector<dagc*>& res){
    dagc* l = root->children[0];
    dagc* r = root->children[1];
    assert(r->isAssigned());

    // abs(x+y) > 3 -> -(x + y) > 3 or (x+y) > 3
    // abs(x+y) > 0 -> x+y != 0
    // abs(x+y) >= 0 -> true
    dagc* content = l->children[0];
    if(root->iseq()){
        res.emplace_back(parser->mk_or(
            parser->mk_eq(content, parser->mk_const(r->v)),
            parser->mk_eq(content, parser->mk_const(-r->v))
        ));
    }
    else if(root->isneq()){
        res.emplace_back(parser->mk_or(
            parser->mk_neq(content, parser->mk_const(r->v)),
            parser->mk_neq(content, parser->mk_const(-r->v))
        ));
    }
    else if(root->isle()){
        if(r->v<0){
            res.emplace_back(parser->mk_false());
        }
        else{
            res.emplace_back(parser->mk_le(content, parser->mk_const(r->v)));
            res.emplace_back(parser->mk_ge(content, parser->mk_const(-r->v)));
        }
    }
    else if(root->islt()){
        if(r->v<=0){
            res.emplace_back(parser->mk_false());
        }
        else{
            res.emplace_back(parser->mk_lt(content, parser->mk_const(r->v)));
            res.emplace_back(parser->mk_gt(content, parser->mk_const(-r->v)));
        }
    }
    else if(root->isge()){
        if(r->v<=0){
            res.emplace_back(parser->mk_true());
        }
        else{
            res.emplace_back(parser->mk_or(
                parser->mk_ge(content, parser->mk_const(r->v)),
                parser->mk_le(content, parser->mk_const(-r->v))
            ));
        }
    }
    else if(root->isgt()){
        if(r->v<0){
            res.emplace_back(parser->mk_true());
        }
        else if(r->v==0){
            res.emplace_back(parser->mk_neq(content, parser->mk_const(0)));
        }
        else{
            res.emplace_back(parser->mk_or(
                parser->mk_gt(content, parser->mk_const(r->v)),
                parser->mk_lt(content, parser->mk_const(-r->v))
            ));
        }
    }
    else assert(false);

}
void prop_rewriter::convert(dagc* root, bool isTop, dagc* var, std::vector<dagc*>& res){
    // isTop and a = b + c, two of a, b, c are assigned, then add the rest to model and todo
    // isTop and a = 3, then add a=3 to model and todo

    // Implementation:
    // accmulate constants, e.g. x + y + z = 3, y = 0, z = 1 -> x + 1 = 3
    // transform, e.g. x + 1 = 3 -> x = 2
    // if isTop add to model and todo
    // else make a new dagc
    dagc* l = root->children[0];
    dagc* r = root->children[1];

    if(l->isAssigned()){
        // swap
        root->CompRev();
        root->children[0] = r;
        root->children[1] = l;
        l = root->children[0];
        r = root->children[1];
    }

    // r->isAssigned()
    Integer val = r->v; // other hand side
    if(l->isabs()){
        convertAbs(root, res);
    }
    else if(l->isadd()){
        Integer acc = 0;
        for(size_t i=0;i<l->children.size();i++){
            if(l->children[i]->isAssigned()) acc += l->children[i]->v;
        }
        val -= acc;
        if(var->isvnum()){
            if(root->iseq()){
                if(isTop){
                    model->set(var, val);
                    todo.emplace_back(var);
                }
                else{
                    res.emplace_back(parser->mk_eq(var, parser->mk_const(val)));
                }
            }
            else if(root->isneq()){
                res.emplace_back(parser->mk_neq(var, parser->mk_const(val)));
            }
            else if(root->isle()){
                res.emplace_back(parser->mk_le(var, parser->mk_const(val)));
            }
            else if(root->islt()){
                res.emplace_back(parser->mk_lt(var, parser->mk_const(val)));
            }
            else if(root->isge()){
                res.emplace_back(parser->mk_ge(var, parser->mk_const(val)));
            }
            else if(root->isgt()){
                res.emplace_back(parser->mk_gt(var, parser->mk_const(val)));
            }
            else assert(false);
        }
        else{
            assert(var->isabs()); // only abs()
            convertAbs(root, res);
        }
    }
    else if(l->issub()){
        // sub -> add, if sub then assert(false)
        assert(false);
    }
    else if(l->ismul()){
        Integer acc = 1;
        for(size_t i=0;i<l->children.size();i++){
            if(l->children[i]->isAssigned()) acc *= l->children[i]->v;
        }
        if(acc==0){
            if(val==0) res.emplace_back(parser->mk_true());
            else res.emplace_back(parser->mk_false());
            return;
        }
        else if(val % acc != 0){
            res.emplace_back(parser->mk_false());
            return;
        }
        val /= acc;
        if(var->isvnum()){
            if(root->iseq()){
                if(isTop){
                    model->set(var, val);
                    todo.emplace_back(var);
                }
                else{
                    res.emplace_back(parser->mk_eq(var, parser->mk_const(val)));
                }
            }
            else if(root->isneq()){
                res.emplace_back(parser->mk_neq(var, parser->mk_const(val)));
            }
            else if(root->isle()){
                if(acc > 0) res.emplace_back(parser->mk_le(var, parser->mk_const(val)));
                else res.emplace_back(parser->mk_ge(var, parser->mk_const(val)));
            }
            else if(root->islt()){
                if(acc > 0) res.emplace_back(parser->mk_lt(var, parser->mk_const(val)));
                else res.emplace_back(parser->mk_gt(var, parser->mk_const(val)));
            }
            else if(root->isge()){
                if(acc > 0) res.emplace_back(parser->mk_ge(var, parser->mk_const(val)));
                else res.emplace_back(parser->mk_le(var, parser->mk_const(val)));
            }
            else if(root->isgt()){
                if(acc > 0) res.emplace_back(parser->mk_gt(var, parser->mk_const(val)));
                else res.emplace_back(parser->mk_lt(var, parser->mk_const(val)));
            }
            else assert(false);
        }
        else{
            assert(var->isabs()); // only abs()
            convertAbs(root, res);
        }
    }
    else if(l->isdiv()){
        // currently not support
        // y / x
        // dagc* y = root->children[0];
        // dagc* x = root->children[1];
    }
    else if(l->ismod()){
        // currently not support
        // y % x
        // dagc* y = root->children[0];
        // dagc* x = root->children[1];

    }
    else assert(false);
    
}


void prop_rewriter::rewriteComp(dagc* root, bool isTop, std::vector<dagc*>& res){
    eval_rewrite(root);
    if(root->isAssigned()){
        if(root->istrue()) res.emplace_back(parser->mk_true());
        else if(root->isfalse()) res.emplace_back(parser->mk_false());
        else assert(false);
        return; // 05 / 30
    }
    // easy check
    evaluatePoly(root->children[0]);
    dagc* x = root->children[0];
    evaluatePoly(root->children[1]);
    dagc* y = root->children[1];
    bool l = x->isAssigned();
    bool r = y->isAssigned();
    if(!l && !r){
        // unknowns are in both sides -> nothing to do
        res.emplace_back(root);
    }
    else if(!l){
        dagc* tmp = nullptr; // single variable
        int len = 0;
        if(!single(tmp, x, len)) { res.emplace_back(root); return;}
        else convert(root, isTop, tmp, res);
    }
    else if(!r){
        dagc* tmp = nullptr; // single variable
        int len = 0;
        if(!single(tmp, y, len)) { res.emplace_back(root); return;}
        else convert(root, isTop, tmp, res);
    }
    else{
        // l && r
        if(evaluate(root, x->v, y->v)) res.emplace_back(parser->mk_true());
        else res.emplace_back(parser->mk_false());
    }
}
void prop_rewriter::propagatePart(dagc* root, bool isTrue, std::vector<dagc*>& res){
    if(root->isAssigned()){
        if(root->istrue()){
            if(!isTrue) state = State::UNSAT;
            res.emplace_back(parser->mk_true());
        }
        else if(root->isfalse()){
            if(isTrue) state = State::UNSAT;
            res.emplace_back(parser->mk_false());
        }
        else assert(false);
    }
    else if(root->isvbool()){
        if(isTrue){
            model->set(root, 1);
        }
        else{
            model->set(root, 0);
        }
        todo.emplace_back(root);
    }
    else if(root->iscomp()){
        std::vector<dagc*> tmp;
        rewriteComp(root, true, tmp);
        if(tmp.size()==0){
            // set model
        }
        else{
            assert(tmp.size() == 1);
            res.emplace_back(tmp[0]);
        }
    }
    else if(root->isletbool()){
        std::vector<dagc*> tmp;
        isTrue?propagateTrue(root->children[0], tmp):propagateFalse(root->children[0], tmp);
        if(tmp.size()!=0){
            // true or false or x < 0 (x + y < z, y = 0, z = 0)
            // or abs(x) < 3 -> x > -3 and x < 3
            for(size_t i=0;i<tmp.size();i++){
                res.emplace_back(tmp[i]);
            }
        }
        else{
            assert(false);
        }
    }
    else if(root->isitebool()){
        // ite b x y
        // b
        std::vector<dagc*> tmp;
        eval_rewrite(root->children[0]);
        dagc* b = root->children[0];
        if(b->istrue() || b->isfalse()){
            std::vector<dagc*> tmp;
            if(b->istrue()){
                // ite true x y -> x
                isTrue?propagateTrue(root->children[1], tmp):propagateFalse(root->children[1], tmp);
            }
            else{
                // ite false x y -> y
                isTrue?propagateTrue(root->children[2], tmp):propagateFalse(root->children[2], tmp);
            }
            
            if(tmp.size()!=0){
                for(size_t i=0;i<tmp.size();i++){
                    res.emplace_back(tmp[i]);
                }
            }
            else{
                assert(false);
            }
        }
        else{
            // b is undef, so can not decide x or y, only rewrite
            // x
            eval_rewrite(root->children[1]);
            // y
            eval_rewrite(root->children[2]);
            res.emplace_back(root);
        }
    }
    else if(root->iseqbool()){
        if(root->children[0] == root->children[1]){
            // easy check
            res.emplace_back(parser->mk_true());
            return;
        }
        eval_rewrite(root->children[0]);
        eval_rewrite(root->children[1]);
        dagc* x = root->children[0];
        dagc* y = root->children[1];

        if((x->istrue() && y->istrue()) || 
        (x->isfalse() && y->isfalse())){
            res.emplace_back(parser->mk_true());
        }
        else if((x->isfalse() && y->istrue()) || 
                (x->istrue() && y->isfalse())){
            res.emplace_back(parser->mk_false());
        }
        else{
            std::vector<dagc*> tmp; 
            if(x->istrue()){
                isTrue?propagateTrue(y, tmp):propagateFalse(y, tmp);
            }
            else if(x->isfalse()){
                isTrue?propagateFalse(y, tmp):propagateTrue(y, tmp);
            }
            else if(y->istrue()){
                isTrue?propagateTrue(x, tmp):propagateFalse(x, tmp);
            }
            else if(y->isfalse()){
                isTrue?propagateFalse(x, tmp):propagateTrue(x, tmp);
            }
            else{
                // x, y are both not assigned
                if(isTrue && x->isvar() && y->isvar()){
                    model->substitute(x, y);
                }
                else res.emplace_back(root);
                return;
            }
            res.insert(res.end(), tmp.begin(), tmp.end());
        }
    }
    else if(root->isneqbool()){
        if(root->children[0] == root->children[1]){
            // easy check
            res.emplace_back(parser->mk_false());
            return;
        }
        eval_rewrite(root->children[0]);
        eval_rewrite(root->children[1]);
        dagc* x = root->children[0];
        dagc* y = root->children[1];

        if((x->istrue() && y->istrue()) || 
        (x->isfalse() && y->isfalse())){
            res.emplace_back(parser->mk_false());
        }
        else if((x->isfalse() && y->istrue()) || 
                (x->istrue() && y->isfalse())){
            res.emplace_back(parser->mk_true());
        }
        else{
            std::vector<dagc*> tmp; 
            if(x->istrue()){
                isTrue?propagateFalse(y, tmp):propagateTrue(y, tmp);
            }
            else if(x->isfalse()){
                isTrue?propagateTrue(y, tmp):propagateFalse(y, tmp);
            }
            else if(y->istrue()){
                isTrue?propagateFalse(x, tmp):propagateTrue(x, tmp);
            }
            else if(y->isfalse()){
                isTrue?propagateTrue(x, tmp):propagateFalse(x, tmp);
            }
            else{
                if(!isTrue && x->isvar() && y->isvar()){
                    model->substitute(x, y);
                    return;
                }
                else res.emplace_back(root);
                return;
            }
            res.insert(res.end(), tmp.begin(), tmp.end());
        }
    }
    else {root->print(); assert(false);}
}

void prop_rewriter::propagateTrue(dagc* root, std::vector<dagc*>& res){
    // given root is true and propagate
    if(root->isAssigned()){
        if(root->istrue()) res.emplace_back(parser->mk_true());
        else if(root->isfalse()){
            state = State::UNSAT;
            res.emplace_back(parser->mk_false());
        }
        else assert(false);
    }
    else{
        if( root->isvbool() || root->isletbool() || root->isitebool() ||
            root->iseqbool() || root->isneqbool() || root->iscomp()){
                propagatePart(root, true, res);
        }
        else if(root->isand()){
            std::vector<dagc*> rep;
            auto it = root->children.begin();
            while(it!=root->children.end()){
                if(!(*it)->isAssigned()){
                    propagateTrue(*it, rep);
                }
                else{
                    if((*it)->istrue()){}
                    else{
                        // isfalse()
                        root->assign(0);
                        state = State::UNSAT;
                        res.emplace_back(parser->mk_false());
                        return;
                    }
                }
                ++it;
            }

            if(rep.size() == 0){
                // all true
                root->assign(1);
                res.emplace_back(parser->mk_true());
            }
            else if(rep.size() == 1){
                res.emplace_back(rep[0]);
            }
            else{
                root->children.clear();
                root->children.assign(rep.begin(), rep.end());
                res.emplace_back(root);
            }
        }
        else if(root->isor()){
            if(root->children.size() == 1){
                // root->children[0] = true
                propagateTrue(root->children[0], res);
            }
            else{
                // can only check 
                eval_rewrite(root);
                if(root->istrue()){
                    res.emplace_back(parser->mk_true());
                }
                else if(root->isfalse()){
                    state = State::UNSAT;
                    res.emplace_back(parser->mk_false());
                }
                else{
                    res.emplace_back(root);
                }
            }
        }
        else if(root->isnot()){
            // nnf rewriter guarantees that 
            // root->parent is not "not" and "not and" / "not or" will never appear
            // so that it will only return tmp.size() <= 1
            dagc* x = root->children[0];
            if(x->isvbool()){
                model->set(x, 0);
                root->assign(1);
                res.emplace_back(parser->mk_true());
            }
            else if(x->isTop()){
                state = State::UNSAT;
                res.emplace_back(parser->mk_false());
            }
            else if(x->isletbool()){
                propagateFalse(x, res);
            }
            else{
                nnfer->rewrite(root);
                propagateTrue(root, res);
            }
        }
        else {root->print(); assert(false);}
    }
}
void prop_rewriter::propagateFalse(dagc* root, std::vector<dagc*>& res){
    // given root is false and propagate
    if(root->isAssigned()){
        if(root->istrue()){
            state = State::UNSAT;
            res.emplace_back(parser->mk_true());
        }
        else if(root->isfalse()) res.emplace_back(parser->mk_false());
        else assert(false);
    }
    else{
        if( root->isvbool() || root->isletbool() || root->isitebool() ||
            root->iseqbool() || root->isneqbool() || root->iscomp()){
                propagatePart(root, false, res);
        }
        else if(root->isand()){
            if(root->children.size() == 1){
                // root->children[0] = true
                propagateFalse(root->children[0], res); // global -> res
            }
            else{
                // can only check
                nnfer->rewrite(root, true);
                eval_rewrite(root);
                if(root->istrue()){
                    res.emplace_back(parser->mk_true());
                }
                else if(root->isfalse()){
                    state = State::UNSAT;
                    res.emplace_back(parser->mk_false());
                }
                else{
                    res.emplace_back(root);
                }
            }
        }
        else if(root->isor()){
            std::vector<dagc*> rep;
            auto it = root->children.begin();
            while(it!=root->children.end()){
                if(!(*it)->isAssigned()){
                    propagateFalse(*it, rep);
                }
                else{
                    if((*it)->istrue()){
                        root->assign(1);
                        // it will conlict, but not happens here
                        state = State::UNSAT;
                        res.emplace_back(parser->mk_true());
                        return;
                    }
                    else{}
                }
                ++it;
            }

            if(rep.size() == 0){
                // all false
                root->assign(0);
                res.emplace_back(parser->mk_false());
            }
            else if(rep.size() == 1){
                res.emplace_back(rep[0]);
            }
            else{
                root->children.clear();
                root->children.assign(rep.begin(), rep.end());
                res.emplace_back(parser->mk_not(root));
            }
        }
        else if(root->isnot()){
            // nnf rewriter guarantees that 
            // root->parent is not "not" and "not and" / "not or" will never appear
            // so that it will only return tmp.size() <= 1
            dagc* x = root->children[0];
            if(x->isvbool()){
                model->set(x, 1);
                root->assign(1);
                res.emplace_back(parser->mk_false());
            }
            else if(x->isTop()){
                // TODO, make false
                res.emplace_back(parser->mk_false());
            }
            else if(x->isletbool()){
                propagateTrue(x, res);
            }
            else{
                nnfer->rewrite(root, true);
                propagateTrue(root, res);
            }
        }
        else {root->print(); assert(false);}
    }
}

void prop_rewriter::eval_rewrite(dagc*& root){
    if(root->isAssigned()){}
    else{
        if(root->isvbool()){}
        else if(root->iscomp()){
            dagc* x = root->children[0];
            dagc* y = root->children[1];
            if(x == y){
                if(root->iseq() || root->isle() || root->isge()){
                    root = parser->mk_true();
                }
                else if(root->isneq() || root->islt() || root->isgt()){
                    root = parser->mk_false();
                }
                else assert(false);
                return;
            }
            else{
                if(x->isAssigned() && y->isAssigned()){
                    root = evaluate(root, x->v, y->v)?parser->mk_true():parser->mk_false();
                }
                else{
                    // use substitute map for simplify
                    dagc* px = model->getSubRoot(x);
                    dagc* py = model->getSubRoot(y);
                    if(px == py){
                        if(root->iseq()){
                            root = parser->mk_true();
                            return;
                        }
                        else if(root->isneq()){
                            root = parser->mk_false();
                            return;
                        }
                    }
                    else if(px->isAssigned() && py->isAssigned()){
                        root = evaluate(root, px->v, py->v)?parser->mk_true():parser->mk_false();
                    }
                }
            }
        }
        else if(root->isand()){
            std::vector<dagc*> tmp;
            for(unsigned i=0;i<root->children.size();i++){
                eval_rewrite(root->children[i]);
                dagc* x = root->children[i];
                
                if(x->isAssigned()){
                    if(x->isfalse()){
                        root = parser->mk_false();
                        return;
                    }
                    else{}
                }
                else if(x->isTop()){}
                else{
                    tmp.emplace_back(x);
                }
            }
            if(tmp.size() == 0){
                root = parser->mk_true();
            }
            else if(tmp.size() == 1){
                root = tmp[0];
            }
            else{ 
                root->children.clear();
                root->children.assign(tmp.begin(), tmp.end());
            }
        }
        else if(root->isor()){
            std::vector<dagc*> tmp;
            for(unsigned i=0;i<root->children.size();i++){
                eval_rewrite(root->children[i]);
                dagc* x = root->children[i];
                
                if(x->isAssigned()){
                    if(x->istrue()){
                        root = parser->mk_true();
                        return;
                    }
                    else{}
                }
                else if(x->isTop()){
                    root = parser->mk_true();
                    return;
                }
                else{
                    tmp.emplace_back(x);
                }
            }

            if(tmp.size() == 0){
                root = parser->mk_false();
            }
            else if(tmp.size() == 1){
                root = tmp[0];
            }
            else{ 
                root->children.clear();
                root->children.assign(tmp.begin(), tmp.end());
            }
        }
        else if(root->isnot()){
            eval_rewrite(root->children[0]);
            dagc* x = root->children[0];
            
            if(x->isAssigned()){
                if(x->istrue()){
                    root = parser->mk_false();
                }
                else{
                    root = parser->mk_true();
                }
            }
            else if(x->isTop()){
                root = parser->mk_false();
            }
        }
        else if(root->isletbool()){
            eval_rewrite(root->children[0]);
            dagc* x = root->children[0];
            
            if(x->isAssigned()){
                if(x->istrue()){
                    root = parser->mk_true();
                }
                else{
                    root = parser->mk_false();
                }
            }
            else if(x->isTop()){
                root = parser->mk_true();
            }
        }
        else if(root->isitebool()){
            eval_rewrite(root->children[0]);
            dagc* x = root->children[0];
            
            if(x->isAssigned()){
                if(x->istrue()){
                    root = root->children[1];
                    eval_rewrite(root);
                }
                else{
                    root = root->children[2];
                    eval_rewrite(root);
                }
            }
            else if(x->isTop()){
                root = root->children[1];
                eval_rewrite(root);
            }
            else{
                // cannot simplify but rewrite x y
                eval_rewrite(root->children[1]);
                eval_rewrite(root->children[2]);
            }
        }
        else if(root->iseqbool()){
            eval_rewrite(root->children[0]);
            eval_rewrite(root->children[1]);
            dagc* x = root->children[0];
            dagc* y = root->children[1];
            if(x->isAssigned() && y->isAssigned()){
                if((x->istrue() && y->istrue()) || 
                (x->isfalse() && y->isfalse())){
                    root = parser->mk_true();
                }
                else if((x->isfalse() && y->istrue()) || 
                        (x->istrue() && y->isfalse())){
                    root = parser->mk_false();
                }
                else assert(false);
            }
        }
        else if(root->isneqbool()){
            eval_rewrite(root->children[0]);
            eval_rewrite(root->children[1]);
            dagc* x = root->children[0];
            dagc* y = root->children[1];
            if(x->isAssigned() && y->isAssigned()){
                if((x->istrue() && y->istrue()) || 
                (x->isfalse() && y->isfalse())){
                    root = parser->mk_false();
                }
                else if((x->isfalse() && y->istrue()) || 
                        (x->istrue() && y->isfalse())){
                    root = parser->mk_true();
                }
                else assert(false);
            }
        }
        else{ root->print(); assert(false); }
    }
}

bool prop_rewriter::rewrite(){
    setTodo();
    // set top constraints
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->iscomp() || (*it)->isletbool()||
                (*it)->iseqbool() || (*it)->isneqbool()){
            (*it)->setTop();
        }
        ++it;
    }

    // remove true constraints
    it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->istrue()) it = data->assert_list.erase(it);
        else{
            ++it;
        }
    }

    boost::unordered_map<dagc*, unsigned> duplicate_atoms;

    while(todo.size()!=0){
        currents.clear();
        duplicate_atoms.clear();
        for(size_t i=0;i<todo.size();i++){
            currents.insert(todo[i]);
        }
        todo.clear();
        it = data->assert_list.begin();
        while(it!=data->assert_list.end()){
            std::vector<dagc*> res;
            propagateTrue(*it, res);
            if(res.size()==1 && res[0]->isAssigned()){
                if(res[0]->isfalse()){
                    state = State::UNSAT;
                    return false;
                }
                else{
                    it = data->assert_list.erase(it);
                }
            }
            else{
                // bool changed = true;
                // if(*it == res[0]){
                //     changed = false;
                // }
                it = data->assert_list.erase(it);
                for(unsigned i=0;i<res.size();i++){
                    // if(changed){
                    //     std::cout<<"origin: ";
                    //     data->printAST(*it);
                    //     std::cout<<" -> "<<std::endl;
                    //     data->printAST(res[i]);
                    //     std::cout<<std::endl;
                    // }
                    if(duplicate_atoms.find(res[i]) != duplicate_atoms.end()){}
                    else{
                        duplicate_atoms.insert(std::pair<dagc*, unsigned>(res[i], 1));
                        data->assert_list.insert(it, res[i]);
                    }
                }
            }
            // std::cout<<"-----------------------"<<std::endl;
        }
    }
    // model->print_partial();
    return state != State::UNSAT;
}

void prop_rewriter::setNNF(nnf_rewriter* nr){
    nnfer = nr;
}
void prop_rewriter::setTodo(){
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->isvbool()){
            model->set(*it, 1);
            it = data->assert_list.erase(it);
        }
        else if((*it)->isnot() && (*it)->children[0]->isvbool()){
            model->set((*it)->children[0], 0);
            it = data->assert_list.erase(it);
        }
        else ++it;
    }
    model->getAssignedVars(todo);
}