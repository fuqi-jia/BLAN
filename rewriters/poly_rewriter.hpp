/* poly_rewriter.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _POLY_REWRITER_H
#define _POLY_REWRITER_H

#include "frontend/dag.hpp"
#include "utils/expoly.hpp"
#include "rewriters/rewriter.hpp"

namespace ismt
{
    class poly_rewriter: public rewriter
    {
    private:
        poly::Context context;
        poly::Assignment zeroAssignment;
        poly::Polynomial zeroPolynomial;

        boost::unordered_map<dagc*, poly::Variable*> varMap;
        std::vector<poly::Polynomial> compPolyList;
        boost::unordered_map<dagc*, unsigned> compPolyMap;  // dagc* -> comp poly index -> poly::Polynomial

        std::string ite_name(dagc* root);
        std::string abs_name(dagc* root);

        void toPolynomial(dagc* root, poly::Polynomial& p);
        dagc* toMonomial(dagc* var, unsigned d);

        dagc* toConstant(Integer& c);
        dagc* toVar(unsigned v);
        dagc* toTerm(unsigned t);
        dagc* toMono(unsigned m);
        dagc* toPoly(unsigned p);
        
        // get coeff of monomial
        Integer coeff(dagc* root);
        // negate the polymomial
        dagc* negate(dagc* root);
        // split into two part
        void split(dagc* pres, dagc*& lres, dagc*& rres);
        int count_let_node(dagc* root, bool& support);
    public:
        poly_rewriter(Parser* p): rewriter(p) {
            zeroPolynomial = poly::Polynomial(context, 0);
            epm = new ep_manager(context);
            std::vector<std::string> vars;
            // set integer variables
            for(size_t i=0;i<parser->dag.vnum_int_list.size();i++){
                dagc* var = parser->dag.vnum_int_list[i];
                vars.emplace_back(var->name);
                vdagcMap.insert(std::pair<std::string, dagc*>(var->name, var));
            }
            epm->set_variables(vars);
        }
        ~poly_rewriter(){}

        // nest polynomial
        bool isMonomial(poly::Polynomial& p);
        poly::Value monomialCoefficient(poly::Polynomial& p);
        poly::Value toValue(poly::Polynomial& p);

        poly::Polynomial zeroPoly();
        poly::Polynomial constZeroPoly();
        poly::Polynomial rewrite(dagc* root);

        // to dagc
        dagc* toDagc(const poly::Polynomial& p);
        dagc* getMainVar(const poly::Polynomial& p);
        void  getVars(const poly::Polynomial& p, std::vector<dagc*>& res);
        void  get_comp_polynomial(dagc* root, poly::Polynomial& p);

        // transform: both have positive coefficients 
        // e.g. x - y > 0 -> x > y
        dagc* transform(dagc* root, poly::Polynomial& res);

        poly::Context& get_context(){ return context; }

        poly::Variable* get_var(dagc* var);

        bool not_support(dagc* root);

    public:
        boost::unordered_map<dagc*, unsigned> polyMap;      // dagc* -> poly index -> poly::Polynomial
        boost::unordered_map<std::string, dagc*> vdagcMap;  // variable name -> dagc*
        boost::unordered_map<unsigned, dagc*> epMap;        // polynomial map
        boost::unordered_map<unsigned, dagc*> epMonoMap;    // monomial map
        boost::unordered_map<unsigned, dagc*> epTermMap;    // term map
        boost::unordered_map<unsigned, dagc*> epVarMap;     // variable map
        // boost::unordered_map<unsigned, dagc*> spVars;       // special variables (ite/abs)

        // expand polynomial
        ep_manager* epm; // for bit blasting
    };
    
} // namespace ismt


#endif