/* blaster_logic.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"

using namespace ismt;

#include <algorithm>


// ------------logic operations------------
literal blaster_solver::Not(const literal&  l){
    if(l==Lit_True()) return Lit_False();
    else if(l==-Lit_True()) return Lit_True();
    else if(l==Lit_False()) return Lit_True();
    else if(l==-Lit_False()) return Lit_False();
    return -l;
}
literal blaster_solver::And(const literal& lit1, const literal& lit2){
    literals lits{ lit1, lit2 };
    return And(lits);
}
literal blaster_solver::Or(const literal& lit1, const literal& lit2){
    literals lits{ lit1, lit2 };
    return Or(lits);
}
literal blaster_solver::And(const literals& lits){
    assert(lits.size()>0);
    // simplify, except for -x and x
    literals copy_lits(lits); std::sort(copy_lits.begin(), copy_lits.end());
    literals ans;
    for(size_t i=0;i<copy_lits.size();i++){
        if(copy_lits[i]==solver->True()) continue;
        else if(copy_lits[i]==-solver->False()) continue;
        else if(copy_lits[i]==solver->False()) return solver->False();
        else if(copy_lits[i]==-solver->True()) return solver->False();
        else if(i!=0&&copy_lits[i]==copy_lits[i-1]) continue;
        else ans.emplace_back(copy_lits[i]);
    }
    if(ans.size()==0) return solver->True();
    else if(ans.size()==1) return ans[0];

    bool_var t = newSatVar();

    // t -> a /\ b <=> ~t \/ a /\ ~t \/ b
    for(size_t i=0;i<ans.size();i++){
        clause c;
        c.emplace_back(-t);
        c.emplace_back(ans[i]);
        addClause(c);
    }

    // a /\ b -> t <=> t \/ ~a \/ ~b
    clause c;
    c.emplace_back(t);
    for(size_t i=0;i<ans.size();i++){
        c.emplace_back(-ans[i]);
    }
    addClause(c);

    return t;
}
literal blaster_solver::Or(const literals& lits){
    assert(lits.size()>0);
    // simplify, except for -x and x
    literals copy_lits(lits);std::sort(copy_lits.begin(), copy_lits.end());
    literals ans;
    for(size_t i=0;i<copy_lits.size();i++){
        if(copy_lits[i]==solver->True()) return solver->True();
        if(copy_lits[i]==-solver->False()) return solver->True();
        else if(copy_lits[i]==solver->False()) continue;
        else if(copy_lits[i]==-solver->True()) continue;
        else if(i!=0&&copy_lits[i]==copy_lits[i-1]) continue;
        else ans.emplace_back(copy_lits[i]);
    }
    if(ans.size()==0) return solver->False();
    else if(ans.size()==1) return ans[0];
    
    bool_var t = newSatVar();

    // t -> a \/ b <=> ~t \/ a \/ b
    clause c;
    c.emplace_back(-t);
    for(size_t i=0;i<ans.size();i++){
        c.emplace_back(ans[i]);
    }
    addClause(c);

    // a \/ b -> t <=> ~a \/ t /\ ~b \/ t
    c.clear();
    for(size_t i=0;i<ans.size();i++){
        c.emplace_back(t);
        c.emplace_back(-ans[i]);
        addClause(c);
        c.clear();
    }

    return t;
}
literal blaster_solver::Ite_bool(literal cond, literal ifp, literal elp){
    // simplify
    if(cond==solver->True()) return ifp;
    else if(cond==solver->False()) return elp;
    else if(ifp==elp) return ifp;

    bool_var t = newSatVar();
    bool_var teqi = EqualBit(t, ifp);
    bool_var teqe = EqualBit(t, elp);
    
    // cond -> teqi
    clause c;
    c.emplace_back(-cond);
    c.emplace_back(teqi);
    addClause(c);

    // ~cond -> teqe
    c.clear();
    c.emplace_back(cond);
    c.emplace_back(teqe);
    addClause(c);

    // teqi \/ teqe
    c.clear();
    c.emplace_back(teqi);
    c.emplace_back(teqe);
    addClause(c);

    return t;
}

