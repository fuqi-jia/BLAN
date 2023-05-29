/* expoly.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _EXPOLY_H
#define _EXPOLY_H

#include "utils/types.hpp"
#include "utils/poly.hpp"
#include <cassert>
#include <vector>
#include <stack>
#include <string>
#include <algorithm>
#include <boost/unordered_map.hpp>

namespace ismt
{
    struct ep_term{
        unsigned variable = 0;
        unsigned exponent = 0;
        ep_term(unsigned var, unsigned exp):variable(var), exponent(exp){}
        ~ep_term(){}

        bool operator==(const ep_term& term) const{return (exponent == term.exponent) && (variable == term.variable); }
        bool operator< (const ep_term& t2){return variable < t2.variable;}
        bool operator> (const ep_term& t2){return variable > t2.variable;}
    };

    inline size_t hash_value(const ep_term& term){
        size_t seed = 0;
        boost::hash_combine(seed, boost::hash_value(term.variable));
        boost::hash_combine(seed, boost::hash_value(term.exponent));
        return seed;
    }

    inline bool operator< (const ep_term& t1, const ep_term& t2){
        return t1.variable < t2.variable; // sort via variable order
    }

    struct ep_mono{
        // index list
        std::vector<unsigned> terms;
        ep_mono(const std::vector<unsigned>& data){
            for(size_t i=0;i<data.size();i++){
                terms.emplace_back(data[i]);
            }
            std::sort(terms.begin(), terms.end());
        }
        ep_mono(const ep_mono& m){
            for(size_t i=0;i<m.terms.size();i++){
                terms.emplace_back(m.terms[i]);
            }
            // m is sorted.
            // std::sort(terms.begin(), terms.end());
        }
        ep_mono(){}
        ~ep_mono(){}
        void sort(){ std::sort(terms.begin(), terms.end()); } 

        bool operator==(const ep_mono& mono) const{
            if(terms.size()==mono.terms.size()){
                for(size_t i=0;i<terms.size();i++){
                    if(terms[i]!=mono.terms[i]) return false;
                }
                return true;
            }
            return false;
        }
    };

    inline bool operator< (const ep_mono& m1, const ep_mono& m2){
        // sort via variable order and size
        if(m1.terms.size() == m2.terms.size()){
            for(size_t i=0;i<m1.terms.size();i++){
                if(m1.terms[i] > m2.terms[i]) return false;
                else if(m1.terms[i] < m2.terms[i]) return true;
            }
            return false;
        }
        else if(m1.terms.size() < m2.terms.size()) return true;
        return false;
    }

    inline size_t hash_value(const ep_mono& mono){
        size_t seed = 0;
        for(size_t i=0;i<mono.terms.size();i++){
            boost::hash_combine(seed, boost::hash_value(mono.terms[i]));
        }
        return seed;
    }

    // dynamic structure: without a hash value.
    struct expoly{
        Integer constant;
        std::vector<Integer> coefficients;
        std::vector<unsigned> monos;
        expoly(std::vector<unsigned>& mo, std::vector<Integer>& co, Integer& cos){
            // insert sort
            monos.resize(mo.size());
            coefficients.resize(coefficients.size());
            unsigned idx = 1;
            for(;idx<mo.size();idx++){
                unsigned key=mo[idx];
                int j=idx-1;
                while((j>=0) && (key<monos[j])){
                        monos[j+1]=monos[j];
                        coefficients[j+1]=coefficients[j];
                        j--;
                }
                monos[j+1]=key;
                coefficients[j+1]=co[idx];
            }
            constant = cos;
        }
        expoly(const expoly& p){
            // insert sort
            for(size_t i=0;i<p.monos.size();i++){
                monos.emplace_back(p.monos[i]);
                coefficients.emplace_back(p.coefficients[i]);
            }
            constant = p.constant;
        }
        expoly(){
            constant = 0;
        }
        ~expoly(){}
        bool operator==(const expoly& p) const{
            if(monos.size()!=p.monos.size() || coefficients.size()!=p.coefficients.size()) return false;
            for(size_t i=0;i<monos.size();i++){
                if(monos[i]!=p.monos.size() || coefficients[i]!=p.coefficients[i]) return false;
            }
            return true;
        }
        void sort(){
            // insert sort
            for(int i=(int)monos.size()-1;i>=0;i--){
                unsigned cur_mono=monos[i];
                Integer cur_coe =coefficients[i];
                int j=i-1;
                while((j>=0) && (cur_mono<monos[j])){
                        monos[j+1]=monos[j];
                        coefficients[j+1]=coefficients[j];
                        j--;
                }
                monos[j+1]=cur_mono;
                coefficients[j+1]=cur_coe;
            }
        }
        bool isConstant(){
            return monos.size() == 0 && coefficients.size() == 0;
        }
    };
    
    // hash under monomial
    // before bit blasting, the final step is expanding the polynomial
    class ep_manager{
    private:
        // polynomial context
        poly::Context& context;

        // nested poly -> expanded poly
        poly::Assignment as; // zero assigment
        void add_ct(expoly& pp, unsigned term, const Integer& coeff){
            // pp += coeff*term
            ep_mono m({ term });
            unsigned res = -1;
            if(mono_map.find(m)!=mono_map.end()){
                res = mono_map[m];
            }
            else{
                res = mono_index;
                mono_pool.emplace_back(m);
                mono_map.insert(std::pair<ep_mono, unsigned>(m, mono_index++));
            }
            pp.monos.emplace_back(res);
            pp.coefficients.emplace_back(coeff);
        }
        void add_pt(expoly& pp, expoly& po, unsigned term){
            // pp += po * term
            // e.g. + ( (x + 1) * y^2 + x*y + 1 ) * z ->
            //      (xy^2 + y^2 + x*y + 1) * z (start from here) ->
            //      xy^2z + y^2z + xyz + z.

            // -> xy^2z + y^2z + xyz
            for(size_t i=0;i<po.monos.size();i++){
                unsigned mono = po.monos[i];
                ep_mono m(mono_pool[mono]);
                m.terms.emplace_back(term);
                m.sort();
                unsigned res = -1;
                if(mono_map.find(m)!=mono_map.end()){
                    res = mono_map[m];
                }
                else{
                    res = mono_index;   
                    mono_pool.emplace_back(m);
                    mono_map.insert(std::pair<ep_mono, unsigned>(m, mono_index++));
                }
                // change, and coefficients not change
                po.monos[i] = res;
            }
            // -> + z
            if(po.constant != 0){
                // can not append term directly, must mono(term). 
                // term is not monomial
                po.coefficients.emplace_back(po.constant);
                ep_mono m;
                m.terms.emplace_back(term);
                m.sort();
                unsigned res = -1;
                if(mono_map.find(m)!=mono_map.end()){
                    res = mono_map[m];
                }
                else{
                    res = mono_index;   
                    mono_pool.emplace_back(m);
                    mono_map.insert(std::pair<ep_mono, unsigned>(m, mono_index++));
                }
                po.monos.emplace_back(res);
                po.constant = 0;
            }

            add(pp, { po });
        }

        // convert
        unsigned toVar(std::string name){
            return var_map[name];
        }
        unsigned toTerm(unsigned var, unsigned degree){
            ep_term t(var, degree);
            if(term_map.find(t)!=term_map.end()){
                return term_map[t];
            }
            else{
                unsigned res = term_index;
                term_pool.emplace_back(t);
                term_map.insert(std::pair<ep_term, unsigned>(t, term_index++));
                return res;
            }
        }
        void add(expoly& pp, const std::vector<expoly>& params){
            // pp += {p1, ..., pn}
            for(size_t i=0;i<params.size();i++){
                for(size_t j=0;j<params[i].monos.size();j++){
                    pp.monos.emplace_back(params[i].monos[j]);
                    pp.coefficients.emplace_back(params[i].coefficients[j]);
                }
                pp.constant += params[i].constant;
            }
        }
        void toUniPoly(expoly& pp, const poly::Polynomial& p){
            // univariate polynomial: x^2 + x + 1
            std::vector<expoly> params;
            unsigned var = toVar(std::string(lp_variable_db_get_name(context.get_variable_db(), poly::main_variable(p).get_internal())));
            
            for (int deg = (int)poly::degree(p); deg >= 0; --deg) {
                expoly pi;
                auto coeff = poly::coefficient(p, deg);
                if(poly::is_constant(coeff)){
                    Integer coe = to_long(poly::evaluate(coeff, as));
                    if(deg == 0){
                        pp.constant = coe; // will be set at constant.
                    }
                    else{
                        unsigned term = toTerm(var, deg);
                        add_ct(pp, term, coe);
                    }
                }
                else assert(false);
                params.emplace_back(pi);
            }

            add(pp, params);
        }
        void toPoly(expoly& pp, const poly::Polynomial& p){
            // poly: ( x + 1 ) y^2 + ( x^2 + 1 ) y + x - 1
            unsigned var = toVar(std::string(lp_variable_db_get_name(context.get_variable_db(), poly::main_variable(p).get_internal())));
            std::vector<expoly> params;

            // deg > 0
            for (int deg = (int)poly::degree(p); deg > 0; --deg) {
                expoly pi;
                auto coeff = poly::coefficient(p, deg);
                if(poly::is_constant(coeff)){
                    // e.g. + 2 x^2 / + 2 (deg == 0)
                    Integer coe = to_long(poly::evaluate(coeff, as));
                    unsigned term = toTerm(var, deg);
                    add_ct(pi, term, coe);
                }
                else if(poly::is_univariate(coeff)){
                    // e.g. + (x + 1) * y -> xy + y.
                    expoly uni;
                    toUniPoly(uni, coeff);
                    unsigned term = toTerm(var, deg);
                    add_pt(pi, uni, term);
                }
                else{
                    // polynomial
                    // e.g. + ( (x + 1) * y^2 + x*y + 1 ) * z -> 
                    //      (xy^2 + y^2 + x*y + 1) * z ->
                    //      xy^2z + y^2z + xyz + z.
                    expoly po;
                    toPoly(po, coeff);
                    unsigned term = toTerm(var, deg);
                    add_pt(pi, po, term);
                }
                params.emplace_back(pi);
            }
            // deg == 0
            auto coeff = poly::coefficient(p, 0);
            if(poly::is_constant(coeff) || poly::is_zero(coeff)){
                Integer coe = to_long(poly::evaluate(coeff, as));
                pp.constant = coe; // will be set at constant.
            }
            else{
                // polynomial
                expoly zero;
                toPoly(zero, coeff);
                params.emplace_back(zero);
            }
            // done
            add(pp, params);
        }

    
    public:
        ep_manager(poly::Context& ctxt): context(ctxt){
            // constant
            // ep_term t(var_index++, 0);
            // term_pool.emplace_back(t);
            // term_map.insert(std::pair<eq_term, unsigned>(t, term_index++));
        }
        ~ep_manager(){}

        // variable pool
        unsigned var_index = 0;
        std::vector<std::string> var_pool;
        boost::unordered_map<std::string, unsigned> var_map; // var -> index

        // term pool
        unsigned term_index = 0;
        std::vector<ep_term> term_pool;
        boost::unordered_map<ep_term, unsigned> term_map; // term -> index

        // monomial pool
        unsigned mono_index = 0;
        std::vector<ep_mono> mono_pool;
        boost::unordered_map<ep_mono, unsigned> mono_map; // mono -> index

        // polynomial pool
        std::vector<expoly> poly_pool;
        std::vector<poly::Polynomial> npoly_pool; // nested poly pool

        // special variables
        std::vector<unsigned> spvar_pool;

        void set_variables(std::vector<std::string>& vars){
            for(size_t i=0;i<vars.size();i++){
                var_pool.emplace_back(vars[i]);
                var_map.insert(std::pair<std::string, unsigned>(vars[i], var_index++));
            }
        }
        unsigned add_special_var(const std::string& var){
            // TODO, to dagc 2022.03.18
            if(var_map.find(var)==var_map.end()){
                var_pool.emplace_back(var);
                unsigned idx = var_index;
                var_map.insert(std::pair<std::string, unsigned>(var, idx));
                spvar_pool.emplace_back(idx);
                ++var_index;
                return idx;
            }
            else{
                return var_map[var];
            }
        }
        unsigned from_poly(const poly::Polynomial& p){
            // only once
            expoly res;
            toPoly(res, p);
            res.sort();
            unsigned idx = poly_pool.size();
            poly_pool.emplace_back(res);
            npoly_pool.emplace_back(p);
            return idx;
        }
        // print
        void print_term(unsigned t){
            ep_term& tt = term_pool[t];
            std::string name = var_pool[tt.variable];
            if(tt.exponent == 1)
                std::cout<<name;
            else
                std::cout<<name<<"^"<<tt.exponent;
        }
        void print_term(ep_term& tt){
            std::string name = var_pool[tt.variable];
            if(tt.exponent == 1)
                std::cout<<name;
            else
                std::cout<<name<<"^"<<tt.exponent;
        }
        void print_mono(ep_mono& mm){
            // print monomial
            for(size_t i=0;i<mm.terms.size();i++){
                print_term(mm.terms[i]);
                if(i!=mm.terms.size()-1)
                    std::cout<<"*";
            }
        }
        void print_mono(unsigned m){
            // print monomial
            ep_mono& mm = mono_pool[m];
            for(size_t i=0;i<mm.terms.size();i++){
                print_term(mm.terms[i]);
                if(i!=mm.terms.size()-1)
                    std::cout<<"*";
            }
        }
        void print_poly(expoly& pp){
            for(size_t i=0;i<pp.monos.size();i++){
                assert(pp.coefficients[i] != 0);
                if(pp.coefficients[i] > 0){
                    if(i==0){
                        if(pp.coefficients[i]!=1)
                            std::cout<<pp.coefficients[0];
                    }
                    else{
                        if(pp.coefficients[i]!=1)
                            std::cout<<"+"<<pp.coefficients[0];
                        else
                            std::cout<<"+";
                    }
                }
                else if(pp.coefficients[i] < 0){
                    if(pp.coefficients[i]!=-1)
                        std::cout<<pp.coefficients[0];
                    else
                        std::cout<<"-";
                }
                print_mono(pp.monos[i]);
            }
        }
        void print_poly(unsigned p){
            // print polynomial
            expoly& pp = poly_pool[p];
            print_poly(pp);
        }
    };
    
} // namespace ismt


#endif

