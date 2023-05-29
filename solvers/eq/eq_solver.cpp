/* eq_solver.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/eq/eq_solver.hpp"

using namespace ismt;

void eq_solver::set(dagc* a, dagc* b){
    if(subMap.find(a)!=subMap.end()){
        std::vector<dagc*>* t = subMap[a];
        t->emplace_back(b);
    }
    else{
        std::vector<dagc*>* t = new std::vector<dagc*>();
        t->emplace_back(b);
        subMap.insert(std::pair<dagc*, std::vector<dagc*>*>(a, t));
    }
}
void eq_solver::collect(dagc* root){
    if(root->isnumop() || root->isvar() || root->isconst()) return;
    else if(root->iseq() || root->iseqbool()){
        dagc* a = root->children[0];
        dagc* b = root->children[1];

        // swap
        if(b->isvar()){
            dagc* t = a;
            a = b;
            b = t;
        }

        if(a->isvar()){
            // prop rewriter guarantees !b->isvar()
            assert(!b->isvar());
            set(a, b);
            if(b->isnumop()){
                set(a, b);
            }
            else if(b->isboolop()){
                set(a, b);
            }
            else assert(false);
        }
        else return; // nothing to do
    }
    else{
        for(size_t i=0;i<root->children.size();i++){
            collect(root->children[i]);
        }
    }
}
void eq_solver::collect(){
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        collect(*it);
        ++it;
    }
}
void eq_solver::rewrite(dagc* root){
    auto it = root->children.begin();
    while(it!=root->children.end()){
        if((*it)->isvar()){
            dagc* parent = model->getSubRoot(*it);
            if (parent != *it){
                *it = parent;
            }
        }
        else{
            rewrite(*it);
        }
        ++it;
    }
}
void eq_solver::rewrite(){
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->isvbool()){
            dagc* parent = model->getSubRoot(*it);
            if (parent != *it){
                *it = parent;
            }
        }
        else{
            rewrite(*it);
        }
        ++it;
    }
}


void eq_solver::setRewriters(eq_rewriter* eq, poly_rewriter* po){
    eqer = eq; polyer = po;
}
void eq_solver::setCollector(Collector* c){ thector = c; }

// bool eq_solver::checkEqAtoms(std::vector<dagc*>& closure){
// }
// bool eq_solver::checkEqPolys(std::vector<dagc*>& closure){
// }
// bool eq_solver::checkEqs(std::vector<dagc*>& closure){
//     if(closure[0]->isvbool()){
//         if(!checkEqAtoms(closure)) return false;
//         else{
//             satser->reset();
//             return true;
//         }
//     }
//     else{
//         // closure[0]->isvnum()
//         return checkEqPolys(closure);
//     }
// }

bool eq_solver::check(){
    // 0) simplify
    // k*x == y && y == k*z
    // -> y == k*z && k*x == k*z
    // -> y == k*z && x == z (final)
    // prop rewriter guarantees both x, z are no constants

    // 1) (= x (+ y 1)) (= y (+ x 1)) -> (= y (+ y 1)) -> false
    // 2) (!= y 0) (= x (* y y)) (= x (- (* y y))) -> (= (* y y) (- (* y y))) -> false
    //      use 2*y*y = 0 -> y = 0, but y=0 not in collector's feasible set.

    // auto it = subMap.begin();
    // while(it!=subMap.end()){
    //     std::vector<dagc*>* t = it->second;
    //     t->emplace(t->begin(), it->first);
    //     if(checkEqs(*t)){
    //         state = State::UNSAT;
    //         return false;
    //     }
    //     ++it;
    // }

    return true;
}
bool eq_solver::solve(){
    collect();
    if(!check()) return false;
    rewrite();
    return true;
}