/* comp_rewriter.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "rewriters/comp_rewriter.hpp"
#include "utils/feasible_set.hpp"
#include <set>
#include <algorithm>

using namespace ismt;

comp_rewriter::comp_rewriter(Parser* p, poly_rewriter* po, eq_rewriter* eq): rewriter(p){
    polyer = po;
    eqer = eq;
}
comp_rewriter::~comp_rewriter(){}

bool allVars(dagc* root){
    // variables are in the same top layer
    for(size_t i=0;i<root->children.size();i++){
        if(!root->children[i]->isvnum()) return false;
    }
    return true;
}

bool comp_rewriter::isNormForm(dagc* root){
    // top level p ~ c, c is a constant
    // next level +
    // next next level *
    // more than one variable
    boost::unordered_set<dagc*> varset;
    if(!root->iscomp()) return false;
    if(!root->children[0]->iscnum() && !root->children[1]->iscnum()) return false;
    if(root->children[0]->iscnum()){
        root = root->children[1];
    }
    else if(root->children[1]->iscnum()){
        root = root->children[0];
    }

    if(!root->isadd()) return false;
    for(unsigned i=0;i<root->children.size();i++){
        dagc* r = root->children[i];
        if(r->ismul()){
            for(unsigned j=0;j<r->children.size();j++){
                if(r->children[j]->isvnum()){
                    varset.insert(r->children[j]);
                }
                else return false;
            }
        }
        else if(r->isvnum()){
            varset.insert(r);
        }
        else return false;
    }
    return varset.size() > 1;
}
bool comp_rewriter::isSimpleComp(dagc* root){
    // x != y -> x - y != 0 which it is more difficult.
    // x = y -> x - y = 0 which it is more difficult.
    dagc* x = root->children[0];
    dagc* y = root->children[1];

    return (
        (x->isvar() && y->isvar()) ||
        (x->isvar() && y->iscnum()) ||
        (x->iscnum() && y->isvar()) 
    ); // mul and var for future.
}
dagc* comp_rewriter::rewrite(dagc* root, bool isTop){
    if(root->isor() || root->isand()){
        bool res = isTop&&root->isand();
        for(size_t i=0;i<root->children.size();i++){
            if(root->children[i]->iscomp()){
                root->children[i] = rewrite(root->children[i], res);
                // eqer try to analyze futher
                // e.g. x = 3 is return and isTop 
                //      then eqer will add it to model
                dagc* eqans = eqer->rewrite(root->children[i], res);
                if(eqans == nullptr){
                    consistent = false;
                    return nullptr;
                }
                else root->children[i] = eqans;
            }
            else rewrite(root->children[i], res);
        }
    }
    else if(root->iscomp()){
        if(isSimpleComp(root)){
            // nothing to do, must stay here
            // else not support will do.
        }
        else if(!polyer->not_support(root)){
            // !isSimpleComp(root)
            curIsTop = isTop;
            // poly::Polynomial lhs = polyer->rewrite(root->children[0]);
            // poly::Polynomial rhs = polyer->rewrite(root->children[1]);
            poly::Polynomial lhs(context, 0);
            polyer->get_comp_polynomial(root, lhs);
            poly::Polynomial rhs(context, 0);

            dagc* res = nullptr;
            if(root->iseq()) res = eq_rewrite(lhs, rhs);
            else if(root->isneq()) res = neq_rewrite(lhs, rhs);
            else if(root->isle()) res = le_rewrite(lhs, rhs);
            else if(root->islt()) res = lt_rewrite(lhs, rhs);
            else if(root->isge()) res = ge_rewrite(lhs, rhs);
            else if(root->isgt()) res = gt_rewrite(lhs, rhs);
            else assert(false);

            // if x = y invoke eqer to set model.
            if(isTop && res->iseq() && allVars(res)){
                res = eqer->rewrite(res);
            }
            return res;
        }
    }
    else{}

    return root;
}

dagc* comp_rewriter::eqZero(dagc* var){
    return parser->mk_eq(var, parser->mk_const(0));
}
dagc* comp_rewriter::orEqZero(std::vector<dagc*> vars){
    std::vector<dagc*> params;
    for(size_t i=0;i<vars.size();i++){
        params.emplace_back(eqZero(vars[i]));
    }
    return parser->mk_or(params);
}
dagc* comp_rewriter::andEqZero(std::vector<dagc*> vars){
    std::vector<dagc*> params;
    for(size_t i=0;i<vars.size();i++){
        params.emplace_back(eqZero(vars[i]));
    }
    return parser->mk_and(params);
}
dagc* comp_rewriter::eq_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs){
    // equal:
    // 1) no-add-mul:
    //  x = 3 -> x = 3
    // 2) only add:
    //  x + 1 = 3 -> x = 2
    // 3) only mul:
    //  3*x = 3 -> x = 1
    //  3*x = 4 -> false
    //  3*x*y = 0 -> x = 0 \/ y = 0 
    //  3*x*y = 3 -> x*y = 1  // (not implement) -> (x = 1 /\ y = 1) \/ (x = -1 /\ y = -1)
    // 4) add-mul: (c!=0, d in (-oo, +oo))
    //  3*x + 1 = 4 -> 3*x = 3 -> x = 1, 3*x + 1 = 5 -> 3*x = 4 -> false
    //  3*(x + 1) = 3 -> 3*x + 3 = 3 -> x = 0
    //  x*y + (-1)*x*y = 0 -> 0 = 0 -> true.
    //  x*y + x*y = 0 -> 2*x*y = 0 -> x*y = 0 -> x = 0 \/ y = 0
    //  x*y + y*z = 0 -> x*y + y*z = 0 // (not implement) -> y = 0 \/ x = -z 
    //  x*y + (-1)*y*z = 0 -> x*y = y*z // (not implement) -> y = 0 \/ x = z
    //  x*y + (-1)*x*y = c -> 0 = c -> false.
    //  x*y + x*y = c -> 2*x*y = c -> x*y = c / 2 // if c % 2 != 0, return false.
    //  x*y + y*z = c -> x*y + y*z = c
    //  u*v + x*y + y*z = d -> u*v + x*y + y*z = d
    //  u*v + x*y + (-1)*y*z = d -> u*v + x*y = y*z +d (negative -> transposition)

    // lhs = constant \/ rhs = constant
    if(poly::is_constant(lhs) && poly::is_constant(rhs)){
        // 1 == 2
        if(lhs==rhs) return parser->mk_true();
        // 2022.06.13
        poly::Assignment a;
        if(poly::evaluate(lhs, a)==poly::evaluate(rhs, a)) return parser->mk_true();
        else return parser->mk_false();
    }
    else if(poly::is_constant(lhs)){
        // c = p
        if(poly::is_univariate(rhs)){
            poly::Polynomial p = rhs - lhs;
            return univariate_eq(p);
        }
        else if(polyer->isMonomial(rhs)){
            poly::Value c = polyer->toValue(lhs);
            poly::Value coe = polyer->monomialCoefficient(rhs);
            if(c==0){
                // x*y = 0
                std::vector<dagc*> res;
                polyer->getVars(rhs, res);
                return orEqZero(res);
            }
            else{
                assert(coe != 0);
                poly::Integer dc = poly::ceil(c);
                poly::Integer dcoe = poly::ceil(coe);
                if(dc % dcoe != 0) return parser->mk_false();
            }
        }
        poly::Polynomial p = rhs - lhs;
        // transforming
        dagc* y = parser->mk_eq(polyer->toDagc(p), parser->mk_const(0));
        return polyer->transform(y, p);
    }
    else if(poly::is_constant(rhs)){
        return eq_rewrite(rhs, lhs);
    }
    else{
        // p1 = p2
        // p = p1 - p2 = 0
        poly::Polynomial p = lhs - rhs;
        poly::Polynomial z(context, 0);
        return eq_rewrite(p, z);
    }

    return nullptr;
}

dagc* comp_rewriter::le_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs){
    // less equal:
    // 1) no-add-mul:
    //  x <= 3 -> x <= 3
    // 2) only add:
    //  x + 1 <= 3 -> x <= 2
    // 3) only mul:
    //  3*x <= 3 -> x <= 1
    //  3*x <= 4 -> x <= 1
    //  -3*x <= 4 -> x >= -1
    //  3*x*y <= 3 -> x*y <= 1
    // 4) add-mul: (c!=0, d in (-oo, +oo))
    //  3*x + 1 <= 4 -> 3*x <= 3 -> x <= 1, 3*x + 1 <= 5 -> 3*x <= 4 -> x <= 1
    //  3*(x + 1) <= 3 -> 3*x + 3 <= 3 -> x <= 0
    //  x*y + (-1)*x*y <= 0 -> 0 <= 0 -> true.
    //  x*y + x*y <= 0 -> 2*x*y <= 0 -> x*y <= 0
    //  x*y + (-1)*y*z <= 0 -> x*y <= y*z
    //  x*y + (-1)*x*y <= c -> 0 <= c -> false.
    //  x*y + x*y <= c -> 2*x*y <= c -> x*y <= c / 2
    //  x*y + y*z <= c -> x*y + y*z <= c
    //  u*v + x*y + y*z <= d -> u*v + x*y + y*z <= d
    //  u*v + x*y + (-1)*y*z <= d -> u*v + x*y <= y*z +d (negative -> transposition)

    // lhs = constant \/ rhs = constant
    if(poly::is_constant(lhs) && poly::is_constant(rhs)){
        // 1 <= 2
        if(lhs<=rhs) return parser->mk_true();
        else return parser->mk_false();
    }
    else if(poly::is_constant(lhs)){
        // c <= p
        if(poly::is_univariate(rhs)){
            poly::Polynomial p = rhs - lhs;
            return univariate_comp(p, true, true);
        }
        poly::Polynomial p = rhs - lhs;
        dagc* y = parser->mk_ge(polyer->toDagc(p), parser->mk_const(0));
        return polyer->transform(y, p);
    }
    else if(poly::is_constant(rhs)){
        // p <= c
        if(poly::is_univariate(lhs)){
            poly::Polynomial p = lhs - rhs;
            return univariate_comp(p, false, true);
        }
        poly::Polynomial p = lhs - rhs;
        dagc* y = parser->mk_le(polyer->toDagc(p), parser->mk_const(0));
        return polyer->transform(y, p);
    }
    else{
        // p1 <= p2
        // p = p1 - p2 <= 0
        poly::Polynomial p = lhs - rhs;
        poly::Polynomial z(context, 0);
        return le_rewrite(p, z);
    }

    return nullptr;
}
dagc* comp_rewriter::lt_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs){
    // less:
    // 1) no-add-mul:
    //  x < 3 -> x < 3
    // 2) only add:
    //  x + 1 < 3 -> x < 2
    // 3) only mul:
    //  3*x < 3 -> x < 1
    //  3*x < 4 -> x < 1
    //  -3*x < 4 -> x > -1
    //  3*x*y < 3 -> x*y < 1
    // 4) add-mul: (c!=0, d in (-oo, +oo))
    //  3*x + 1 < 4 -> 3*x < 3 -> x < 1, 3*x + 1 < 5 -> 3*x < 4 -> x < 1
    //  3*(x + 1) < 3 -> 3*x + 3 < 3 -> x < 0
    //  x*y + (-1)*x*y < 0 -> 0 < 0 -> true.
    //  x*y + x*y < 0 -> 2*x*y < 0 -> x*y < 0
    //  x*y + (-1)*y*z < 0 -> x*y < y*z
    //  x*y + (-1)*x*y < c -> 0 <= c -> false.
    //  x*y + x*y < c -> 2*x*y < c -> x*y < c / 2
    //  x*y + y*z < c -> x*y + y*z < c
    //  u*v + x*y + y*z < d -> u*v + x*y + y*z < d
    //  u*v + x*y + (-1)*y*z < d -> u*v + x*y < y*z +d (negative -> transposition)
    
    // lhs = constant \/ rhs = constant
    if(poly::is_constant(lhs) && poly::is_constant(rhs)){
        // 1 < 2
        if(lhs<rhs) return parser->mk_true();
        else return parser->mk_false();
    }
    else if(poly::is_constant(lhs)){
        // c < p
        if(poly::is_univariate(rhs)){
            poly::Polynomial p = rhs - lhs;
            return univariate_comp(p, true, false);
        }
        poly::Polynomial p = rhs - lhs;
        dagc* y = parser->mk_gt(polyer->toDagc(p), parser->mk_const(0));
        return polyer->transform(y, p);
    }
    else if(poly::is_constant(rhs)){
        // p < c
        if(poly::is_univariate(lhs)){
            poly::Polynomial p = lhs - rhs;
            return univariate_comp(p, false, false);
        }
        poly::Polynomial p = lhs - rhs;
        dagc* y = parser->mk_lt(polyer->toDagc(p), parser->mk_const(0));
        return polyer->transform(y, p);
    }
    else{
        // p1 < p2
        // p = p1 - p2 < 0
        poly::Polynomial p = lhs - rhs;
        poly::Polynomial z(context, 0);
        return lt_rewrite(p, z);
    }
}
dagc* comp_rewriter::ge_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs){
    return le_rewrite(rhs, lhs);
}
dagc* comp_rewriter::gt_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs){
    return lt_rewrite(rhs, lhs);
}
dagc* comp_rewriter::neq_rewrite(poly::Polynomial& lhs, poly::Polynomial& rhs){
    // convert to eq and then flip
    return parser->mk_not(eq_rewrite(lhs, rhs));
}
dagc* comp_rewriter::univariate_eq(poly::Polynomial& p){
    dagc* var = polyer->getMainVar(p);
    std::vector<Value> answers;
    // if(integer) roots_isolate_int(p, answers);
    // else roots_isolate(p, answers);
    roots_isolate_int(p, answers);
    dagc* res = nullptr;
    if(answers.size()==0) return parser->mk_false();
    else if(answers.size()==1){
        res = parser->mk_eq(var, parser->mk_const(to_long(answers[0])));
    }
    else{
        std::vector<dagc*> params;
        for(size_t i=0;i<answers.size();i++){
            params.emplace_back(parser->mk_eq(var, parser->mk_const(to_long(answers[i]))));
        }
        res = parser->mk_or(params);
    }
    res->setIntervalLit();
    return res;
}
dagc* comp_rewriter::univariate_comp(poly::Polynomial& p, bool g, bool e){
    // p(x) $ c, $: { <, <=, >=, > }.
    dagc* var = polyer->getMainVar(p);
    std::vector<Interval> answers;
    // if(integer) roots_isolate_comp_int(p, g, e, answers);
    // else roots_isolate_comp(p, g, e, answers);
    roots_isolate_comp_int(p, g, e, answers);

    if(answers.size()==0) return parser->mk_false();
    else if(answers.size()==1&& isFull(answers[0])) return parser->mk_true();
    else{
        std::vector<dagc*> params;
        for(size_t i=0;i<answers.size();i++){
            dagc* tmp = nullptr;
            if(isFull(answers[i])){ assert(false); }
            else if(isPoint(answers[i])){
                tmp = parser->mk_eq(var, parser->mk_const(to_long(getRealUpper(answers[i]))));
            }
            else if(ninf(answers[i])){
                tmp = parser->mk_le(var, parser->mk_const(to_long(getRealUpper(answers[i]))));
            }
            else if(pinf(answers[i])){
                tmp = parser->mk_ge(var, parser->mk_const(to_long(getRealLower(answers[i]))));
            }
            else{
                // [x, y]
                tmp = parser->mk_and(
                    parser->mk_ge(var, parser->mk_const(to_long(getRealLower(answers[i])))),
                    parser->mk_le(var, parser->mk_const(to_long(getRealUpper(answers[i]))))
                );
            }
            params.emplace_back(tmp);
        }
        dagc* res = nullptr;
        if(params.size()==1){
            res = params[0];
        }
        else{
            res = parser->mk_or(params);
        }
        res->setIntervalLit();
        return res;
    }
}


void comp_rewriter::roots_isolate_int(poly::Polynomial& p, std::vector<Value>& rets){
    poly::UPolynomial up = poly::to_univariate(p);
    std::vector<poly::AlgebraicNumber> vals(poly::isolate_real_roots(up));
    for(size_t i=0;i<vals.size();i++){
        if(poly::is_integer(vals[i])){
            // integer, ceil == floor
            rets.emplace_back(poly::ceil(vals[i]));
        }
    }
}

bool comp_rewriter::evaluate(poly::UPolynomial& up, bool g, bool e, poly::Integer& val){
    if(g && e){
        // p >= 0
        poly::Integer ans = poly::evaluate_at(up, val);
        if(ans >= 0){
            return true;
        }
    }
    else if(g && !e){
        // p > 0
        poly::Integer ans = poly::evaluate_at(up, val);
        if(ans > 0){
            return true;
        }
    }
    else if(!g && e){
        // p <= 0
        poly::Integer ans = poly::evaluate_at(up, val);
        if(ans <= 0){
            return true;
        }
    }
    else{
        // !g & !e
        // p < 0
        poly::Integer ans = poly::evaluate_at(up, val);
        if(ans < 0){
            return true;
        }
    }
    return false;
}
void comp_rewriter::roots_isolate_comp_int(poly::Polynomial& p, bool g, bool e, std::vector<Interval>& rets){
    poly::UPolynomial up = poly::to_univariate(p);
    std::vector<poly::AlgebraicNumber> vals(poly::isolate_real_roots(up));
    if(vals.size()==0){
        auto val = poly::evaluate_at(up, 0);
        assert(val != 0);
        if( (val > 0 && g) || (val < 0 && !g) ) rets.emplace_back(FullInterval);
        return;
    }

    std::sort(vals.begin(), vals.end());
    poly::Integer preval;
    poly::Integer One(1);
    // std::cout<<up<<std::endl;
    feasible_set* fs = new feasible_set();

    // std::vector<Interval> ians;
    for(size_t i=0;i<vals.size();i++){
        bool is_integer = poly::is_integer(vals[i]);
        if(i == 0){
            // (-oo, vals[0])
            poly::Integer val = poly::floor(vals[i]);
            Interval itv = Interval(Value::minus_infty(), true, val, true);
            bool checked = true;
            poly::Integer check_value = get_inner_value(itv, checked);
            if(checked && evaluate(up, g, e, check_value)){
                fs->addInterval(itv);
            }
        }
        if(i == vals.size() - 1){
            // (vals[size-1], +oo)
            poly::Integer val = poly::ceil(vals[i]);
            Interval itv = Interval(val, true, Value::plus_infty(), true);
            bool checked = true;
            poly::Integer check_value = get_inner_value(itv, checked);
            if(checked && evaluate(up, g, e, check_value)){
                fs->addInterval(itv);
            }
        }

        if(i != 0){
            // (vals[i-1], vals[i])
            bool is_pre_integer = poly::is_integer(vals[i-1]);
            poly::Integer preval = poly::ceil(vals[i-1]);
            poly::Integer val = poly::floor(vals[i]);
            if(!is_pre_integer && !is_integer){
                Interval itv = Interval(preval, false, val, false);
                bool checked = true;
                poly::Integer check_value = get_inner_value(itv, checked);
                if(checked && evaluate(up, g, e, check_value)){
                    fs->addInterval(itv);
                }
            }
            else if(!is_pre_integer){
                Interval itv = Interval(preval, false, val, true);
                bool checked = true;
                poly::Integer check_value = get_inner_value(itv, checked);
                if(checked && evaluate(up, g, e, check_value)){
                    fs->addInterval(itv);
                }
            }
            else if(!is_integer){
                Interval itv = Interval(preval, true, val, false);
                bool checked = true;
                poly::Integer check_value = get_inner_value(itv, checked);
                if(checked && evaluate(up, g, e, check_value)){
                    fs->addInterval(itv);
                }
            }
            else{
                Interval itv = Interval(preval, true, val, true);
                bool checked = true;
                poly::Integer check_value = get_inner_value(itv, checked);
                if(checked && evaluate(up, g, e, check_value)){
                    fs->addInterval(itv);
                }
            }
        }
        
        if(is_integer){
            poly::Integer val = poly::floor(vals[i]);
            if(evaluate(up, g, e, val)){
                fs->addInterval(Interval(val, false, val, false));
            }
        }
    }

    // std::cout<<(*fs)<<std::endl;

    fs->get_intervals(rets);
    delete fs;
}

bool comp_rewriter::rewrite(){
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->iscomp()){
            dagc* t = rewrite(*it, true);
            if(!consistent){
                return false;
            }
            *it = t;
        }
        else rewrite(*it);
        ++it;
    }
    return true;
}
