/* nnf_rewriter.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "rewriters/nnf_rewriter.hpp"

using namespace ismt;

bool nnf_rewriter::isVar(dagc* node){
    return node->isvbool() || node->isletbool();
}

// convert to nnf-like tree
void nnf_rewriter::toNNFNode(dagc*& node, bool isNot){
    // escape cases:
    if(node->isvnum() || node->iscnum() || node->isnumop() || node->isitenum() ||
            node->isfunint() || node->isletvar()) return;
    // check cases:
    if(node->isnot()){
        // TODO
        assert(node->children.size()==1);
        if( isVar(node->children[0]) ) return;
        // 2023-05-18
        node = node->children[0];
        toNNFNode(node, !isNot);
        // 2023-05-18 done
        // node->t = NT_AND;
        // node->name = "and";
        // toNNFNode(node->children[0], !isNot);
    }
    else if(isNot){ // isNot = true
        if( isVar(node) ){
            std::cout<<"node->isvbool() || node->children[0]->isletbool()"<<std::endl;
            assert(false);
        }
        else if(node->iscbool()){
            // const boolean
            if(node->v==parser->mk_true()->v){
                node->id = 0;
                node->v = 0;
                node->name = "false";
            }
            else{
                node->id = 1;
                node->v = 1;
                node->name = "true";
            }
            return;
        }
        else if(node->iseqbool() || node->isneqbool() || node->iscomp()){
            node->CompNot();
        }
        else if(node->isand()){
            node->t = NT_OR;
            node->name = "or";
            for(size_t i=0;i<node->children.size();i++){
                if( isVar(node->children[i]) ){ // not lit
                    node->children[i] = parser->mk_not(node->children[i]);
                }
                else if(node->children[i]->isnot()){ // not not lit -> lit
                    node->children[i] = node->children[i]->children[0];
                    toNNFNode(node->children[i], false);
                }
                else toNNFNode(node->children[i], isNot);
            }
        }
        else if(node->isor()){
            node->t = NT_AND;
            node->name = "and";
            for(size_t i=0;i<node->children.size();i++){
                if( isVar(node->children[i]) ){ // not lit
                    node->children[i] = parser->mk_not(node->children[i]);
                }
                else if(node->children[i]->isnot()){ // not not lit
                    node->children[i] = node->children[i]->children[0];
                    toNNFNode(node->children[i], false);
                }
                else toNNFNode(node->children[i], isNot);
            }
        }
        else if(node->isitebool()){
            assert(node->children.size()==3);
            if( isVar(node->children[1]) ){ // not lit
                node->children[1] = parser->mk_not(node->children[1]);
            }
            else if(node->children[1]->isnot()){ // not not lit
                node->children[1] = node->children[1]->children[0];
                toNNFNode(node->children[1], false);
            }
            else toNNFNode(node->children[1], isNot);

            if( isVar(node->children[2]) ){ // not lit
                node->children[2] = parser->mk_not(node->children[2]);
            }
            else if(node->children[2]->isnot()){ // not not lit
                node->children[2] = node->children[2]->children[0];
                toNNFNode(node->children[2], false);
            }
            else toNNFNode(node->children[2], isNot);
        }
        else if(node->islet()){
            dagc* ret = node;
            while(ret->islet()){
                // first simplify all the let bool nodes
                for(unsigned i=1;i<ret->children.size();i++){
                    if(ret->children[i]->isletbool()){
                        toNNFNode(ret->children[i]->children[0], false);
                    }
                }
                // go on find out let content
                ret = ret->children[0];
            }
            toNNFNode(ret, isNot);
        }
        else{
            std::cout<<"toNNFNode error!"<<std::endl;
            node->print(), assert(false);
        }
    }
    else{ // isNot = false
        if(node->islet()){
            // process let
            dagc* ret = node;
            while(ret->islet()){
                // first simplify all the let bool nodes
                for(unsigned i=1;i<ret->children.size();i++){
                    if(ret->children[i]->isletbool()){
                        toNNFNode(ret->children[i]->children[0], false);
                    }
                }
                // go on find out let content
                ret = ret->children[0];
            }
            toNNFNode(ret, isNot);
        }
        else{
            for(size_t i=0;i<node->children.size();i++){
                toNNFNode(node->children[i], isNot);
            }
        }
    }
}

void nnf_rewriter::rewrite(dagc*& root, bool isNot){
    toNNFNode(root, isNot);
}

// note that finally (not letvar) is permitted.
void nnf_rewriter::rewrite(){
    
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        rewrite(*it);
        ++it;
    }
}