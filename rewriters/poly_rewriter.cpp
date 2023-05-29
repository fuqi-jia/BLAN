/* poly_rewriter.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/


#include "rewriters/poly_rewriter.hpp"
#include <stack>
#include <set>

using namespace ismt;

// TODO, special variables
// std::string poly_rewriter::ite_name(dagc* root){
//     std::string res = "";
//     return res;
// }
// std::string poly_rewriter::abs_name(dagc* root){
//     std::string res = "";
//     return res;
// }

poly::Variable* poly_rewriter::get_var(dagc* var){
    if(varMap.find(var)!=varMap.end()) return varMap[var];
    else{
        poly::Variable* x = new poly::Variable(context, var->name.c_str());
        varMap.insert(std::pair<dagc*, poly::Variable*>(var, x));
        return x;
    }
}
void poly_rewriter::toPolynomial(dagc* root, poly::Polynomial& p){
    // traverse the tree to get polynomial and add variable to the context.
    if(root->isvnum()){
        // poly::Variable x(context, root->name.c_str()); will be a different variables
        if(varMap.find(root)!=varMap.end()) p += *varMap[root];
        else{
            poly::Variable* x = new poly::Variable(context, root->name.c_str());
            varMap.insert(std::pair<dagc*, poly::Variable*>(root, x));
            p += *x;
        }
    }
    else if(root->iscnum()){
        p = poly::Polynomial(context, poly::Integer(root->v)) + p;
    }
    else if(root->isadd()){
        for(size_t i=0;i<root->children.size();i++){
            poly::Polynomial pp(context);
            toPolynomial(root->children[i], pp);
            p = pp + p;
        }
    }
    else if(root->ismul()){
        p = 1;
        for(size_t i=0;i<root->children.size();i++){
            poly::Polynomial pp(context);
            toPolynomial(root->children[i], pp);
            p = pp * p; // TODO, p * pp will be error...
        }
    }
    else if(root->isletvar()){
        poly::Polynomial pp(context);
        toPolynomial(root->children[0], pp);
        p = pp + p;
    }
    else { root->print(), assert(false); }

    // TODO, how to deal with ite/abs/...
}

bool poly_rewriter::isMonomial(poly::Polynomial& p){
    if(poly::is_constant(p)) return true;
    else{
        // new added 
        unsigned d = poly::degree(p);
        std::vector<poly::Polynomial> vec;
        for(int k=d;k>=0;k--){
            poly::Polynomial pp = poly::coefficient(p, k);
            if(poly::is_constant(pp) && poly::is_zero(pp)){}
            else{
                vec.emplace_back(pp);
            }
        }

        if(vec.size()!=1) return false;
        else{
            return isMonomial(vec[0]);
        }
    }
}
poly::Value poly_rewriter::monomialCoefficient(poly::Polynomial& p){
    if(poly::is_constant(p)) return toValue(p);
    else{
        // new added 
        unsigned d = poly::degree(p);
        std::vector<poly::Polynomial> vec;
        for(int k=d;k>=0;k--){
            poly::Polynomial pp = poly::coefficient(p, k);
            if(poly::is_constant(pp) && poly::is_zero(pp)){}
            else{
                vec.emplace_back(pp);
            }
        }

        if(vec.size()!=1) assert(false);
        else return monomialCoefficient(vec[0]);
    }
}
poly::Value poly_rewriter::toValue(poly::Polynomial& p){
    return poly::evaluate(p, zeroAssignment);
}

poly::Polynomial poly_rewriter::zeroPoly(){
    return poly::Polynomial(context);
}
poly::Polynomial poly_rewriter::constZeroPoly(){
    return zeroPolynomial;
}
poly::Polynomial poly_rewriter::rewrite(dagc* root){
    poly::Polynomial p(context); // p = 0
    // if p = x*y then p init as 1
    if(root->ismul()) p = 1;
    // else p = x*y + z then p init as 0
    toPolynomial(root, p);
    return p;
}
// TODO, ite/div/mod not support

int poly_rewriter::count_let_node(dagc* root, bool& support){
    if(root->isite() || root->isabs() || root->isdiv() || root->ismod()){
        support = false;
        return -1;
    }
    int ans = 0;
    if(root->children.size() == 0) return 1;
    for(unsigned i=0;i<root->children.size();i++){
        ans += count_let_node(root->children[i], support);
        if(!support){
            return -1;
        }
    }
    return ans;
}
bool poly_rewriter::not_support(dagc* root){
    bool ans = false;
    for(size_t i=0;i<root->children.size();i++){
        dagc* x = root->children[i];
        if(x->isite() || x->isabs() || x->isdiv() || x->ismod()) return true;
        else if(x->isletnum()){
            bool support = true;
            int num = count_let_node(x, support);
            if(!support) return true;
            if(num > 64) return true; // too big
        }
        else{
            ans |= not_support(x);
            if(ans) return true;
        }
    }
    return false;
}

// to dagc
dagc* poly_rewriter::toVar(unsigned v){
    // TODO, special variables
    return vdagcMap[epm->var_pool[v]];
}
dagc* poly_rewriter::toConstant(Integer& c){
    return parser->mk_const(c);
}
dagc* poly_rewriter::toTerm(unsigned t){
    if(epTermMap.find(t)!=epTermMap.end()){
        return epTermMap[t];
    }
    else{
        // TODO, Jia a new idea? now not support
        // now only simple check, if x^4 then check whether there exist x^2, x^3
        // it depends the order.
        ep_term& term = epm->term_pool[t];
        
        dagc* var = toVar(term.variable);
        std::vector<dagc*> params;
        for(size_t i=0;i<term.exponent;i++){
            params.emplace_back(var);
        }
        assert(params.size()>0);
        dagc* res = nullptr;
        if(params.size()==1) res = params[0];
        else res = parser->mk_mul(params);
        epTermMap.insert(std::pair<unsigned, dagc*>(t, res));
        return res;
        // devide ...
        // ep_term rec(0, 0);
        // dagc* tterm = nullptr;
        // for(size_t i=term.exponent;i>1;i--){
        //     ep_term tmp(term.variable, i);
        //     if(epm->term_map.find(tmp)!=epm->term_map.end()){
        //         rec.variable = tmp.variable;
        //         rec.exponent = tmp.exponent;
        //         tterm = epTermMap[epm->term_map[tmp]];
        //         break;
        //     }
        // }
        // if(tterm!=nullptr){
        //     unsigned times = term.exponent / rec.exponent;
        //     unsigned rest = term.exponent - rec.exponent * times;
        //     std::vector<dagc*> params;
        //     // term = rec^times * var^rest
        //     for(size_t i=0;i<term.exponent;i++){
        //         params.emplace_back(tterm);
        //     }
        //     if(rest != 0){
        //         dagc* var = toVar(term.variable);
        //         for(size_t i=0;i<term.exponent;i++){
        //             params.emplace_back(var);
        //         }
        //     }
        //     assert(params.size()>0);
        //     dagc* res = nullptr;
        //     if(params.size()==1) res = params[0];
        //     else res = parser->mk_mul(params);
        //     epTermMap.insert(std::pair<unsigned, dagc*>(t, res));
        //     return res;
        // }
        // else{
        //     dagc* var = toVar(term.variable);
        //     std::vector<dagc*> params;
        //     for(size_t i=0;i<term.exponent;i++){
        //         params.emplace_back(var);
        //     }
        //     assert(params.size()>0);
        //     dagc* res = nullptr;
        //     if(params.size()==1) res = params[0];
        //     else res = parser->mk_mul(params);
        //     epTermMap.insert(std::pair<unsigned, dagc*>(t, res));
        //     return res;
        // }
    }
}
dagc* poly_rewriter::toMono(unsigned m){
    if(epMonoMap.find(m)!=epMonoMap.end()){
        return epMonoMap[m];
    }
    else{
        ep_mono& mono = epm->mono_pool[m];
        std::vector<dagc*> params;
        // TODO
        // x^4y^2z = x^4y^2 * z, but now x^4 * y^2 * z seperately.
        for(size_t i=0;i<mono.terms.size();i++){
            params.emplace_back(toTerm(mono.terms[i]));
        }
        assert(params.size()>0);
        dagc* res = nullptr;
        if(params.size()==1) res = params[0];
        else res = parser->mk_mul(params);
        epMonoMap.insert(std::pair<unsigned, dagc*>(m, res));
        return res;
    }
}
dagc* poly_rewriter::toPoly(unsigned p){
    // Integer constant;
    // std::vector<Integer> coefficients;
    // std::vector<unsigned> monos;
    expoly& pp = epm->poly_pool[p];
    std::vector<dagc*> params;
    for(size_t i=0;i<pp.monos.size();i++){
        dagc* m = toMono(pp.monos[i]);
        dagc* c = toConstant(pp.coefficients[i]);
        if(c->v==1) params.emplace_back(m);
        else if(c->v==0) continue;
        else params.emplace_back(parser->mk_mul(c, m));
    }
    if(pp.constant != 0) params.emplace_back(parser->mk_const(pp.constant));
    assert(params.size()>0);
    dagc* res = nullptr;
    if(params.size()==1) res = params[0];
    else res = parser->mk_add(params);
    epMap.insert(std::pair<unsigned, dagc*>(p, res));
    return res;
}
dagc* poly_rewriter::toDagc(const poly::Polynomial& p){
    unsigned ans = epm->from_poly(p);
    if(epMap.find(ans)!=epMap.end()){
        return epMap[ans];
    }
    else{
        // generate dagc*
        dagc* res = toPoly(ans);
        polyMap.insert(std::pair<dagc*, unsigned>(res, ans));
        return res;
    }
}
void poly_rewriter::get_comp_polynomial(dagc* root, poly::Polynomial& p){
    assert(root->iscomp());
    if(compPolyMap.find(root)!=compPolyMap.end()){
        p = compPolyList[compPolyMap[root]];
    }
    else{
        poly::Polynomial p1(context);
        poly::Polynomial p2(context);
        toPolynomial(root->children[0], p1);
        toPolynomial(root->children[1], p2);
        poly::Polynomial res = p1 - p2;
        polyMap.insert(std::pair<dagc*, unsigned>(root, compPolyList.size()));
        compPolyList.emplace_back(res);
        p = res;
    }
}
dagc* poly_rewriter::getMainVar(const poly::Polynomial& p){
    std::string name = std::string(lp_variable_db_get_name(context.get_variable_db(), poly::main_variable(p).get_internal()));
    return vdagcMap[name];
}
void poly_rewriter::getVars(const poly::Polynomial& p, std::vector<dagc*>& res){
    poly::VariableCollector vc;
    vc(p);
    std::vector<poly::Variable> vars(vc.get_variables());
    for(size_t i=0;i<vars.size();i++){
        std::string name = std::string(lp_variable_db_get_name(context.get_variable_db(), vars[i].get_internal()));
        res.emplace_back(vdagcMap[name]);
    }
}

Integer poly_rewriter::coeff(dagc* root){
    if(root->iscnum()) return root->v;
    for(size_t i=0;i<root->children.size();i++){
        dagc* x = root->children[i];
        if(x->isconst()) return x->v;
    }
    return 1;
    // if(root->isvnum()) return 1;
    // else if(root->iscnum()) return root->v;
    // else{
    //     Integer c = 1;
    //     for(size_t i=0;i<root->children.size();i++){
    //         c *= coeff(root->children[i]);
    //     }
    //     return c;
    // }
}
dagc* poly_rewriter::negate(dagc* root){
    if(root->isconst()) return parser->mk_const(-root->v);
    std::vector<dagc*> res;
    for(size_t i=0;i<root->children.size();i++){
        dagc* x = root->children[i];
        if(x->isvnum() || x->ismul()){
            res.emplace_back(x);
        }
        else if(x->isconst()){
            if(x->v == -1){
                // -x->v = 1, so escape
            }
            else{
                // new const dagc
                res.emplace_back(parser->mk_const(-x->v));
            }
        }
        else assert(false);
    }
    
    if(res.size()==0){
        assert(false);
    }
    else if(res.size()==1){
        return res[0];
    }
    else return parser->mk_mul(res);
}
void poly_rewriter::split(dagc* pres, dagc*& lres, dagc*& rres){
    // split with two sides, both are positive coefficients
    std::vector<dagc*> lefs;
    std::vector<dagc*> rigs;
    for(size_t i=0;i<pres->children.size();i++){
        dagc* p = pres->children[i];
        Integer c = coeff(p);
        if(c > 0) lefs.emplace_back(p);
        else if(c < 0) rigs.emplace_back(negate(p));
    }
    
    if(lefs.size()==0) lres = parser->mk_const(0);
    else if(lefs.size()==1) lres = lefs[0];
    else lres = parser->mk_add(lefs);

    if(rigs.size()==0) rres = parser->mk_const(0);
    else if(rigs.size()==1) rres = rigs[0];
    else rres = parser->mk_add(rigs);
}
dagc* poly_rewriter::transform(dagc* root, poly::Polynomial& res){
    assert(root->iscomp());
    dagc* pres = toDagc(res);
    if(pres->ismul()){
        // is monomial and then nothing to do.
        std::vector<dagc*> params;
        Integer c = coeff(pres);
        if(c<0){
            pres = negate(pres);
            root->children[0] = root->children[1];
            root->children[1] = pres;
        }
        else if(c==0){
            root->children[0] = parser->mk_const(0);
        }
        else{
            // nothing to do
        }
    }
    else{
        // finite sum of terms and then split into two part ( + / - ).
        dagc* lres = nullptr;
        dagc* rres = nullptr;
        split(pres, lres, rres);
        assert(lres && rres);
        root->children[0] = lres;
        root->children[1] = rres;
    }
    return root;
}