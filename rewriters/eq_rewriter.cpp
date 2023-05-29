/* eq_rewriter.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "rewriters/eq_rewriter.hpp"

using namespace ismt;

void eq_rewriter::setModel(model_s* m){ model = m; }
// TODO substitute
// TODO not support local model
dagc* eq_rewriter::rewrite(dagc* root, bool isTop){
    if(root->iscbool() || root->istrue() || root->isfalse()){}
    else if(isTop){
        if(root->iseq()){
            dagc* a = root->children[0];
            dagc* b = root->children[1];
            if(a->iscnum() && b->iscnum()){
                // a == b
                if(a->v!=b->v){
                    consistent = false;
                    return parser->mk_false();
                }
                else return parser->mk_true();
            }
            else if(a->iscnum() && b->isvnum()){
                // v = x
                if(model->set(b, a->v)) return parser->mk_true();
                else{
                    consistent = false;
                    return parser->mk_false();
                }
            }
            else if(a->isvnum() && b->iscnum()){
                // x = v
                if(model->set(a, b->v)) return parser->mk_true();
                else{
                    consistent = false;
                    return parser->mk_false();
                }
            }
            else if(a->isvnum() && b->isvnum()){
                // x = y

                // assume substitute in model and globalSubSet is the same.
                // so that the union sets are the same.
                if(model->substitute(a, b)) return parser->mk_true();
                else{
                    consistent = false;
                    return parser->mk_false();
                }
            }
        }
        else if(root->iseqbool()){
            dagc* a = root->children[0];
            dagc* b = root->children[1];
            if(a->iscbool() && b->iscbool()){
                // a == b
                if(a->v!=b->v){
                    consistent = false;
                    return parser->mk_false();
                }
                else return parser->mk_true();
            }
            else if(a->iscbool() && b->isvbool()){
                // v = x
                if(model->set(b, a->v)) return parser->mk_true();
                else{
                    consistent = false;
                    return parser->mk_false();
                }
            }
            else if(a->isvbool() && b->iscbool()){
                // x = v
                if(model->set(a, b->v)) return parser->mk_true();
                else{
                    consistent = false;
                    return parser->mk_false();
                }
            }
            else if(a->isvbool() && b->isvbool()){
                // x = y
                if(model->substitute(a, b)) return parser->mk_true();
                else{
                    consistent = false;
                    return parser->mk_false();
                }
            }
        }
        else{
            std::vector<dagc*>::iterator it = root->children.begin();
            if(root->isvbool()){
                model->set(root, 1);
                return parser->mk_true();
            }
            else if(root->isand()){
                while(it!=root->children.end()){
                    *it = rewrite(*it, isTop);
                    ++it;
                }
            }
            else if(root->isor()){
                // nothing to do
            }
            else if(root->isnot()){
                // data->printAST(root);
                // std::cout<<std::endl;
                // (*it)->print();
                assert( (*it)->iscomp() || 
                        (*it)->isvbool() ||
                        (*it)->iseqbool() ||
                        (*it)->isneqbool() ||
                        (*it)->isletbool()
                );
                // let bool is permitted
                if(!(*it)->isvbool() && !(*it)->isletbool()){
                    *it = rewrite(*it, true);
                    if((*it)->istrue()){
                        return parser->mk_false();
                    }
                    else if((*it)->isfalse()){
                        return parser->mk_true();
                    }
                    // else nothing to do
                }
                // else (*it)->isvbool()
            }
            else if(root->iscomp()){
                // not eq then nothing to do
            }
            else if(root->isletbool()){
                *it = rewrite(*it, true);
                if((*it)->istrue()){
                    return parser->mk_false();
                }
                else if((*it)->isfalse()){
                    return parser->mk_true();
                }
            }
            else {root->print(); assert(false);}
        }
    }
    else{
        // in and
        if(root->isand()){
            disjoint_set<dagc*> localSubset;
            boost::unordered_map<dagc*, Integer> localAssigned;
            root = localRewrite(root, localSubset, localAssigned);
        }
        else{
            for(size_t i=0;i<root->children.size();i++){
                rewrite(root->children[i], false);
            }
        }
    }
    return root;
}
// TODO local rewrite

bool eq_rewriter::localAssign(boost::unordered_map<dagc*, Integer>& localAssigned, dagc* root, Integer val){
    if(localAssigned.find(root)!=localAssigned.end()){
        if(localAssigned[root]!=val) return false;
        else return true;
    }
    else if(model->has(root) && model->getValue(root)!=val) return false; // conflict with top assignment
    else localAssigned.insert(std::pair<dagc*, Integer>(root, val));
    return true;
}
bool eq_rewriter::localSubsitute(disjoint_set<dagc*>& localSubset, dagc* a, dagc* b){
    localSubset.add(a);
    localSubset.add(b);
    localSubset.union_set(a, b);
    return true;
}
dagc* eq_rewriter::localRewrite(
    dagc* root, 
    disjoint_set<dagc*>& localSubset, 
    boost::unordered_map<dagc*, Integer>& localAssigned){
    // or(0) - and(1) - or(2) - and(3), we analyze starting from (1) to (3).
    // for example, (or ... (and (= x 3) (= x 4) (or (= (+ x y) 8) (= (+ x y) 6)))).
    // i.e. ... \/ (x = 3 /\ y = 4 /\ (x + y = 8 \/ x + y = 6)) -> ... \/ false -> ...
    // currently not supported, we implement a more powerful analysis procedure in poly solver's rewriters.
    
    // note that higher assignment will be vaild in lower assignment,
    // lower assignment is not available in higher assignment.

    auto it = root->children.begin();
    if(root->iseq()){
        dagc* a = root->children[0];
        dagc* b = root->children[1];
        if(a->iscnum() && b->iscnum()){
            // a == b
            if(a->v!=b->v) return parser->mk_false();
            *it = parser->mk_true();
        }
        else if(a->iscnum() && b->isvnum()){
            // v = x
            if(localAssign(localAssigned, b, a->v)){}
            else{
                return parser->mk_false();
            }
        }
        else if(a->isvnum() && b->iscnum()){
            // x = v
            if(localAssign(localAssigned, a, b->v)){}
            else{
                return parser->mk_false();
            }
        }
        else if(a->isvnum() && b->isvnum()){
            // x = y

            // assume substitute in model and globalSubSet is the same.
            // so that the union sets are the same.
            localSubsitute(localSubset, a, b);
        }
    }
    else if(root->iseqbool()){
        dagc* a = root->children[0];
        dagc* b = root->children[1];
        if(a->iscbool() && b->iscbool()){
            // a == b
            if(a->v!=b->v){
                return parser->mk_false();
            }
            *it = parser->mk_true();
        }
        else if(a->iscbool() && b->isvbool()){
            // v = x
            if(localAssign(localAssigned, b, a->v)){}
            else{
                return parser->mk_false();
            }
        }
        else if(a->isvbool() && b->iscbool()){
            // x = v
            if(localAssign(localAssigned, a, b->v)){}
            else{
                return parser->mk_false();
            }
        }
        else if(a->isvbool() && b->isvbool()){
            // x = y
            localSubsitute(localSubset, a, b);
        }
    }
    else if(root->isand()){
        // first find eq / eqbool
        while(it!=root->children.end()){
            if((*it)->iseq() && (*it)->children.size()==2){
                *it = localRewrite(*it, localSubset, localAssigned);
            }
            else if((*it)->iseqbool() && (*it)->children.size()==2){
                *it = localRewrite(*it, localSubset, localAssigned);
            }
            ++it;
        }
        // then check consistency at the same level
        it = root->children.begin();
        std::vector<dagc*> rep;
        while(it!=root->children.end()){
            if(localAssigned.find(*it) != localAssigned.end()){
                if((*it)->istrue()){} // nothing to do
                else if((*it)->isfalse()) return parser->mk_false();
            }
            rep.emplace_back(localRewrite(*it, localSubset, localAssigned));
            ++it;
        }
        root->children.clear();
        root->children.assign(rep.begin(), rep.end());
    }
    else if(root->isor()){
        std::vector<dagc*> rep;
        while(it!=root->children.end()){
            if(localAssigned.find(*it) != localAssigned.end()){
                if((*it)->istrue()) return parser->mk_true();
                else if((*it)->isfalse()) {} // nothing to do
            }
            rep.emplace_back(localRewrite(*it, localSubset, localAssigned));
            ++it;
        }
        root->children.clear();
        root->children.assign(rep.begin(), rep.end());
    }
    else if(root->isnot()){
        if(localAssigned.find(*it) != localAssigned.end()){
            if((*it)->istrue()) return parser->mk_false();
            else if((*it)->isfalse()) return parser->mk_true();
        }
        // else now not support?
    }
    else if(root->iscomp()){
        // evaluate assignment
        // now not support?
    }

    return root;
}


bool eq_rewriter::rewrite(){
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->isconst()){}
        else if((*it)->iseq() || (*it)->iseqbool()){
            dagc* t = rewrite(*it, true);
            if(!consistent){
                return false;
            }
            *it = t;
        }
        else *it = rewrite(*it);
        ++it;
    }
    return true;
}