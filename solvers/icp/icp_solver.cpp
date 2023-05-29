/* icp_solver.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/icp/icp_solver.hpp"

using namespace ismt;


icp_solver::icp_solver(poly_rewriter* p):polyer(p){
    data = new poly::IntervalAssignment(p->get_context());
    controller = new sat_solver();
}
icp_solver::~icp_solver(){
    delete controller; controller = nullptr;
    delete data; data = nullptr;
}

bool icp_solver::isEasyComp(dagc* c){
    return c->children[0]->isvnum() && c->children[1]->isvnum();
}
bool icp_solver::easyComp(dagc* c){
    Interval a(data->get(*(polyer->get_var(c->children[0]))));
    Interval b(data->get(*(polyer->get_var(c->children[1]))));
    if(c->iseq()){
        return overlap(a, b);
    }
    else if(c->isneq()){
        return !(poly::is_point(a) && poly::is_point(b) && getRealLower(a) == getRealLower(b));

    }
    else if(c->isge()){
        // a >= b
        return !(!pinf(a) && !ninf(b) && getRealUpper(a) < getRealLower(b));
    }
    else if(c->isgt()){
        // a > b
        return !(!pinf(a) && !ninf(b) && getRealUpper(a) <= getRealLower(b));
    }
    else if(c->isle()){
        // a <= b
        return !(!ninf(a) && !pinf(b) && getRealLower(a) > getRealUpper(b));
    }
    else if(c->islt()){
        // a < b
        return !(!ninf(a) && !pinf(b) && getRealLower(a) >= getRealUpper(b));
    }
    else assert(false);
    return false;
}
// solving functions
bool icp_solver::evaluate(dagc* c){
    if(c->iscomp()){
        // escape this case
        if(polyer->not_support(c)) return true;
        if(isEasyComp(c)){
            return easyComp(c);
        }
        poly::Polynomial pres(polyer->get_context());
        polyer->get_comp_polynomial(c, pres);
        Interval res(poly::evaluate(pres, *data));
        if(c->iseq()){
            return poly::sgn(res) == 0; // contains 0
        }
        else if(c->isneq()){
            // [-2, 3] or [1, 3]
            return (poly::sgn(res) == 0 && !poly::is_point(res)) || poly::sgn(res) != 0;
        }
        else if(c->islt()){
            return (poly::sgn(res) == 0 && !poly::is_point(res)) || poly::sgn(res) < 0;
        }
        else if(c->isle()){
            return poly::sgn(res) <= 0;
        }
        else if(c->isgt()){
            return (poly::sgn(res) == 0 && !poly::is_point(res)) || poly::sgn(res) > 0;
        }
        else if(c->isge()){
            return poly::sgn(res) >= 0;
        }
        else assert(false);
    }
    else if(c->isite()){
        assert(false);
    }
    else{
        if(c->isand()){
            for(unsigned i=0;i<c->children.size();i++){
                if(!evaluate(c->children[i])) return false;
            }
            return true;
        }
        else if(c->isor()){
            for(unsigned i=0;i<c->children.size();i++){
                if(evaluate(c->children[i])) return true;
            }
            return false;
        }
        else if(c->isnot()){
            if(!evaluate(c->children[0])) return true;
            else return false;
        }
        else {c->print(); assert(false);}
    }
    return true;
}
State icp_solver::check(std::vector<dagc*>& cs){
    stored_core.clear();
    for(unsigned i=0;i<cs.size();i++){
        if(cs[i]->iscomp() && !evaluate(cs[i])){
            return State::UNSAT;
        }
        else stored_core.emplace_back(cs[i]);
    }
    return State::SAT;
}
// unsat core if failed -> lemma
void icp_solver::core(std::vector<dagc*>& cores){
    cores.clear();
    cores.assign(stored_core.begin(), stored_core.end());
}
// add operations
void icp_solver::assign(dagc* var, const poly::Interval& v){
    poly::Variable* variable = polyer->get_var(var);
    data->set(*variable, v);
}

void icp_solver::reset(){
    data->clear();
}