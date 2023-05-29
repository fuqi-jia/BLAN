/* logic_rewriter.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "rewriters/logic_rewriter.hpp"

using namespace ismt;

void logic_rewriter::rewrite(dagc* root){
    if(root->isand()){
        // and-and-and-... -> and  
        std::vector<dagc*>::iterator it = root->children.begin();
        std::vector<dagc*> rep;
        while(it!=root->children.end()){
            if( (*it)->isand() ){
                std::vector<dagc*> lres;
                andRecord(*it, lres);
                if(lres.size()>0){
                    for(size_t i=0;i<lres.size();i++){
                        rep.emplace_back(lres[i]);
                    }
                }
                else assert(false); // and null
                ++it;
            }
            else if( (*it)->isor() ){
                rep.emplace_back(*it);
                rewrite(*it); // and-or
                ++it;
            }
            else{
                rep.emplace_back(*it);
                ++it; // next
            }
        }
        root->children.clear();
        root->children.assign(rep.begin(), rep.end());
    }
    else if(root->isor()){
        // or-or-or-... -> or   
        std::vector<dagc*>::iterator it = root->children.begin();
        std::vector<dagc*> rep;
        while(it!=root->children.end()){
            if( (*it)->isor() ){
                std::vector<dagc*> lres;
                orRecord(*it, lres);
                if(lres.size()>0){
                    for(size_t i=0;i<lres.size();i++){
                        rep.emplace_back(lres[i]);
                    }
                }
                else assert(false); // or null
                ++it;
            }
            else if( (*it)->isand() ){
                rep.emplace_back(*it);
                rewrite(*it); // or-and
                ++it;
            }
            else{
                rep.emplace_back(*it);
                ++it; // next
            }
        }
        root->children.clear();
        root->children.assign(rep.begin(), rep.end());
    }
    else{
        std::vector<dagc*>::iterator it = root->children.begin();
        while(it!=root->children.end()){
            rewrite(*it);
            ++it;
        }
    }
}

void logic_rewriter::andRecord(dagc* root, std::vector<dagc*>& res){
    if(root->isand()){
        // and-and-and-... -> and   
        for(size_t i=0;i<root->children.size();i++){
            if(root->children[i]->isand()){
                andRecord(root->children[i], res);
            }
            else{
                res.emplace_back(root->children[i]);
            }
        }
    }
}

void logic_rewriter::orRecord(dagc* root, std::vector<dagc*>& res){
    // or-or-or-... -> or   
    for(size_t i=0;i<root->children.size();i++){
        if(root->children[i]->isor()){
            orRecord(root->children[i], res);
        }
        else{
            res.emplace_back(root->children[i]);
        }
    }
}

void logic_rewriter::rewrite(){
    //  1) eliminate top and
    auto it = data->assert_list.begin();
    std::vector<dagc*> rep;
    while(it!=data->assert_list.end()){
        std::vector<dagc*> res;
        andRecord(*it, res);
        if(res.size()>0){
            for(size_t i=0;i<res.size();i++){
                rep.emplace_back(res[i]);
            }
            ++it;
        }
        else {
            rep.emplace_back(*it);
            ++it; 
        }
    }
    data->assert_list.clear();
    data->assert_list.assign(rep.begin(), rep.end());
    //  2) eliminate the others.
    it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        assert(!(*it)->isand()); // never and
        rewrite(*it); // rewrite all, e.g. (= x (or (or x y) z))
        ++it;
    }
}