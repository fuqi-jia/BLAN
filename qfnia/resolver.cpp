/* resolver.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "qfnia/resolver.hpp"

using namespace ismt;

Resolver::Resolver(/* args */){}
Resolver::~Resolver(){}

void Resolver::init(Info* i){
    info = i;
    parser = info->prep->parser;
    polyer = info->prep->polyer;
    nnfer = info->prep->nnfer;
}

dagc* Resolver::lemma(){
    std::vector<dagc*> params;
    for(size_t i=0;i<core.size();i++){
        params.emplace_back(parser->mk_not(core[i]));
    }
    return parser->mk_or(params);
}

bool Resolver::is_hard(dagc* constraint){
    if( constraint->isor() || constraint->isite() || 
        constraint->isabs() || constraint->isdiv()){
        return true;
    }
    else if(constraint->isvar() || constraint->isconst()) return false;
    else{
        for(size_t i=0;i<constraint->children.size();i++){
            if(is_hard(constraint->children[i])) return true;
        }
        return false;
    }
}
bool Resolver::is_hard(std::vector<dagc*>& constraints){
    // not an or
    for(size_t i=0;i<constraints.size();i++){
        if(is_hard(constraints[i])) return true;
    }
    return false;
}
bool Resolver::has_lemma() const{
    return core.size() != 0;
}
// mcsat use different sovlers
bool Resolver::resolve(){
    // 1. cad
    // 2. just return the negation intervals (now not permitted)
    //      so it will only increment the default width
    // core.clear();
    // if(!is_hard(info->constraints)){
    //     cad_resolve(info->constraints);
    // }
    // else{
    //     auto it = info->assignment.begin();
    //     while(it != info->assignment.end()){
    //         core.emplace_back(it->first);
    //         ++it;
    //     }
    // }

    return true;
}
// bool Resolver::cad_resolve(std::vector<dagc*>& constraints){
//     // cad projection


//     // make cell literals


//     // make backtrack level


//     // next check whether the conflict resolved.
//     return true;
// }