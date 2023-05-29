/* let_rewriter.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "rewriters/let_rewriter.hpp"

using namespace ismt;


// TODO, 2022-05-20, note that we left letvarnum
// get pure content without letvar of letvar
dagc* let_rewriter::content(dagc* root){
    dagc* ret = root->children[0];
    // while(ret->isletvar()){
    while(ret->isletbool()){
        ret = ret->children[0];
    }
    analyzeOp(ret);
    return ret;
}
// check children of root that without letvar
void let_rewriter::analyzeOp(dagc* root){
    for(size_t i=0;i<root->children.size();i++){
        dagc* x = root->children[i];
        // if(x->isletvar()){
        if(x->isletbool()){
            root->children[i] = content(x);
        }
        else analyzeOp(x);
    }
}
// return true -> set all values of children (so maybe res.size()==0)
// return false -> without set model
bool let_rewriter::propagate(dagc* root, std::vector<dagc*>& res){
    if(root->isvbool()){
        // set to model
        model->set(root, 1);
        return true;
    }
    else if(root->isletvar()){
        if(root->isletbool()){
            dagc* x = root->children[0];
            // nodes has 2 cases:
            //  1. append to res
            //  2. set value
            if(propagate(x, res) && x->isAssigned()){ 
                // if all assigned, then assign root.
                // $1 = and a b -> $1 = true, a = true, b = true; {}
                assert(x->isAssigned());
                root->assign(x->v);
                return true;
            }
            else{
                // currently, 
                // $1 = and a>1 b -> b = true; {a>1}
                // $1 = or a b -> ; {(or a b)}
                // BoolOprMap will find out the let var content
            }
            return false;
        }
        else{
            // root->isletnum()
            assert(false);
        }
    }
    else if(root->isand()){
        // check (children) + propagate
        std::vector<dagc*> tmp;
        for(size_t i=0;i<root->children.size();i++){
            dagc* x = root->children[i];
            if(propagate(x, res)){
                // new assigned value
                if(x->isAssigned()){
                    if(x->isfalse()){
                        res.emplace_back(parser->mk_false());
                        root->assign(0);
                        return true;
                    }
                    else{
                        // istrue() -> nothing to do
                    }
                }
                else{
                    tmp.emplace_back(x);
                }
            }
        }
        // // have not assigned
        // if(!ans) res.emplace_back(root);
        root->children.clear();
        if(tmp.size()==0){
            root->children.emplace_back(parser->mk_true());
            return true;
        }
        else{
            // tmp size == 1 will erase in logic operator
            root->children.assign(tmp.begin(), tmp.end());
            res.emplace_back(root);
        }
    }
    else if(root->isor()){
        // only check without propagate
        for(size_t i=0;i<root->children.size();i++){
            dagc* x = root->children[i];
            if(x->isAssigned()){
                if(x->istrue()){
                    root->assign(1);
                    res.emplace_back(parser->mk_true());
                    return true;
                }
                else{}
            }
        }
        res.emplace_back(root);
    }
    else if(root->isnot()){
        if(root->children[0]->isvbool()){
            model->set(root->children[0], 0);
            root->assign(1);
            return true;
        }
        else if(root->children[0]->isletbool()){
            dagc* x = root->children[0];
            
            // only check without propagate
            if(x->isAssigned()){
                if(x->istrue()){
                    root->assign(0);
                    res.emplace_back(parser->mk_false());
                    return true;
                }
                else{
                    root->assign(1);
                    res.emplace_back(parser->mk_true());
                    return true;
                }
            }

            res.emplace_back(root);
        }
        else{
            // after nnf rewriter, never (not operator) exists
            res.emplace_back(root);
        }
    }
    else{
        res.emplace_back(root);
    }
    return false;
}

void let_rewriter::rewrite(dagc* root, std::vector<dagc*>& res, bool isTop){
    if(!isTop) return; // nothing to do
    // let->children[0] = expr, e.g. x+y+1 -> a+10+b+10+1
    // let->children[1..n] = letvar, e.g. x (->a+10), y (->b+10)
    dagc* ret = root;
    // find content
    while(ret->islet()){
        ret = ret->children[0];
    }
    // try to propagate
    propagate(ret, res);
}

bool let_rewriter::rewrite(){
    boost::unordered_map<dagc*, unsigned> duplicate_atoms;
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->islet()){
            // TODO, only top-let will be eliminated
            std::vector<dagc*> res;
            rewrite(*it, res);

            if(res.size()==1 && res[0]->isAssigned()){
                if(res[0]->isfalse()){
                    return false;
                }
                else{
                    it = data->assert_list.erase(it);
                }
            }
            else{
                it = data->assert_list.erase(it);
                for(unsigned i=0;i<res.size();i++){
                    if(duplicate_atoms.find(res[i]) != duplicate_atoms.end()){}
                    else{
                        duplicate_atoms.insert(std::pair<dagc*, unsigned>(res[i], 1));
                        data->assert_list.insert(it, res[i]);
                    }
                }
            }
        }
        else ++it;
    }
    return true;
}