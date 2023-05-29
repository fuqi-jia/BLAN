/* checker.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "qfnia/checker.hpp"
#include <iostream>

using namespace ismt;

#define checkDebug 0

// ------------check operations------------
bool checker::check(dagc* dag){
    return checkAtoms(dag)==1;
}
bool checker::check(){
    auto it = info->message->constraints.begin();
    while(it!=info->message->constraints.end()){
        #if checkDebug
            info->message->data->printAST(*it);
            std::cout<<std::endl;
        #endif
        if(!check(*it)) return false;
        ++it;
    }
    return true;
}

Integer checker::getValue(dagc* root){
    return info->message->model->getValue(root);
}
Integer checker::checkLetNumTerms(dagc* dag){
    if(dag->children[0]->isnumop()) return checkMathTerms(dag->children[0]);
    else if(dag->children[0]->isitenum()) return checkIteNums(dag->children[0]);
    else if(dag->children[0]->isvar()) return getValue(dag->children[0]);
    else if(dag->children[0]->iscnum()) return dag->children[0]->v;
    else if(dag->children[0]->isletnum()) return checkLetNumTerms(dag->children[0]);
    else{ std::cout<<"Check Error let type (in let operations)! "<<std::endl; assert(false);}
    return 0;
}
int checker::checkLetBoolTerms(dagc* dag){
    return checkAtoms(dag->children[0]);
}
int checker::checkAtoms(dagc* dag){
    if(dag->isand()||dag->isor()||dag->isnot()||dag->isitebool()||dag->iseqbool()||dag->isneqbool()){
        std::vector<int> parameters;
        for(size_t i=0;i<dag->children.size();i++){
            dagc* tmp = dag->children[i];
            if(tmp->isboolop()) parameters.emplace_back(checkAtoms(tmp));
            else if(tmp->isnumop()) parameters.emplace_back(checkMathAtoms(tmp));
            else if(tmp->isvbool()) parameters.emplace_back(getValue(tmp)==1?1:0);
            else if(tmp->iscbool()) parameters.emplace_back(tmp->v==1?1:0);
            else if(tmp->isitebool()) parameters.emplace_back(checkAtoms(tmp));
            else if(tmp->isletbool()) parameters.emplace_back(checkLetBoolTerms(tmp));
            else if(tmp->iseqbool()) parameters.emplace_back(checkAtoms(tmp));
            else if(tmp->isneqbool()) parameters.emplace_back(checkAtoms(tmp));
            else{ tmp->print(); std::cout<<"Check Error dag node type (in logic atoms)! "<<std::endl; assert(false); }
        }
        if(dag->isand()){
            #if checkDebug
                std::cout<<"( And( ";
                for(size_t i=0;i<parameters.size()-1;i++){
                    std::cout<<parameters[i]<<" , ";
                }
                std::cout<<parameters[parameters.size()-1]<<" ) )"<<std::endl;
            #endif
            for(size_t i=0;i<parameters.size();i++){
                if(parameters[i]==0) return 0;
            }
            return 1;
        }
        else if(dag->isor()){
            #if checkDebug
                std::cout<<"( Or( ";
                for(size_t i=0;i<parameters.size()-1;i++){
                    std::cout<<parameters[i]<<" , ";
                }
                std::cout<<parameters[parameters.size()-1]<<" ) )"<<std::endl;
            #endif
            for(size_t i=0;i<parameters.size();i++){
                if(parameters[i]==1) return 1;
            }
            return 0;
        }
        else if(dag->isnot()){
            assert(parameters.size()==1);
            #if checkDebug
                // debug: 2021.11.26, 1 - parameters[0].
                std::cout<<"( Not( "<<parameters[0]<<" ) )"<<std::endl;
            #endif
            return 1-parameters[0];
        }
        else if(dag->isitebool()){
            assert(parameters.size()==3);
            #if checkDebug
                std::cout<<"( if ( "<<parameters[0]<<" ) then "<<parameters[1]<<" else "<<parameters[2]<<" )"<<std::endl;
            #endif
            if(parameters[0]==1) return parameters[1];
            else return parameters[2];
        }
        else if(dag->iseqbool()){
            assert(parameters.size()==2);
            #if checkDebug
                std::cout<<"( ( "<<parameters[0]<<" ) == ( "<<parameters[1]<<" ) )"<<std::endl;
            #endif
            return parameters[0]==parameters[1]?1:0;
        }
        else if(dag->isneqbool()){
            assert(parameters.size()==2);
            #if checkDebug
                std::cout<<"( ( "<<parameters[0]<<" ) != ( "<<parameters[1]<<" ) )"<<std::endl;
            #endif
            return parameters[0]!=parameters[1]?1:0;
        }
        else std::cout<<"Check Error logic operatios (in logic atoms)! "<<std::endl, assert(false);
    }
    else if(dag->iscomp()){
        return checkMathAtoms(dag);
    }
    else if(dag->iscbool()){
        #if checkDebug
            std::cout<<"( "<<(dag->v==1?1:0)<<" )"<<std::endl;
        #endif
        return dag->v==1?1:0;
    }
    else if(dag->isvbool()){
        #if checkDebug
            std::cout<<"( "<<getValue(dag)<<" )"<<std::endl;
        #endif
        return getValue(dag)==1?1:0;
    }
    else if(dag->islet()){
        return checkAtoms(dag->children[0]);
    }
    else if(dag->isletbool()){
        return checkAtoms(dag->children[0]);
    }
    else{ std::cout<<"Check Error logic operatios! "<<std::endl; assert(false); }
    return 0;
}
int checker::checkMathAtoms(dagc* dag){
    assert(dag->iscomp());
    std::vector<Integer> parameters;
    for(size_t i=0;i<dag->children.size();i++){
        dagc* tmp = dag->children[i];
        if(tmp->isnumop()) parameters.emplace_back(checkMathTerms(tmp));
        else if(tmp->isvar()) parameters.emplace_back(getValue(tmp));
        else if(tmp->iscnum()) parameters.emplace_back(tmp->v);
        else if(tmp->isitenum()) parameters.emplace_back(checkIteNums(tmp)); // debug: 2021.08.22
        else if(tmp->isletnum()) parameters.emplace_back(checkLetNumTerms(tmp));
        else std::cout<<"Check Error dag node type (in math atoms)! "<<std::endl, assert(false);
    }
    assert(parameters.size()==2);
    if(dag->iseq()){
        assert(parameters.size()==2);
        #if checkDebug
            std::cout<<"( "<<parameters[0]<<" == "<<parameters[1]<<" )"<<std::endl;
        #endif
        return parameters[0] == parameters[1]?1:0;
    }
    else if(dag->isneq()){
        assert(parameters.size()==2);
        #if checkDebug
            std::cout<<"( "<<parameters[0]<<" != "<<parameters[1]<<" )"<<std::endl;
        #endif
        return parameters[0] != parameters[1]?1:0;
    }
    else if(dag->isle()){
        assert(parameters.size()==2);
        #if checkDebug
            std::cout<<"( "<<parameters[0]<<" <= "<<parameters[1]<<" )"<<std::endl;
        #endif
        return parameters[0]<=parameters[1]?1:0;
    }
    else if(dag->islt()){
        assert(parameters.size()==2);
        #if checkDebug
            std::cout<<"( "<<parameters[0]<<" < "<<parameters[1]<<" )"<<std::endl;
        #endif
        return parameters[0]<parameters[1]?1:0;
    }
    else if(dag->isge()){
        assert(parameters.size()==2);
        #if checkDebug
            std::cout<<"( "<<parameters[0]<<" >= "<<parameters[1]<<" )"<<std::endl;
        #endif
        return parameters[0]>=parameters[1]?1:0;
    }
    else if(dag->isgt()){
        assert(parameters.size()==2);
        #if checkDebug
            std::cout<<"( "<<parameters[0]<<" > "<<parameters[1]<<" )"<<std::endl;
        #endif
        return parameters[0]>parameters[1]?1:0;
    }
    else{ std::cout<<"Check Error math atom operatios! "<<std::endl; assert(false);}
    return 0;
}
Integer checker::checkIteNums(dagc* dag){
    assert(dag->isitenum());
    int cond = 0;
    assert(dag->children[0]->isbool()||dag->children[0]->iscbool());
    if(dag->children[0]->iscbool()){
        cond = dag->children[0]->v==1?1:0;
    }
    else{
        if(dag->children[0]->isboolop() || dag->children[0]->isletbool()){
            // debug: 2021.08.22, cond may be a nested formula.
            // debug: 2021.08.25, cond may be a let bool variable.
            cond = checkAtoms(dag->children[0]);
        }
        else{
            cond = getValue(dag->children[0])==1?1:0;
        }
    }
    std::vector<Integer> parameters;
    for(size_t i=1;i<dag->children.size();i++){
        dagc* tmp = dag->children[i];
        if(tmp->isnumop()) parameters.emplace_back(checkMathTerms(tmp));
        else if(tmp->isvar()) parameters.emplace_back(getValue(tmp));
        else if(tmp->iscnum()) parameters.emplace_back(tmp->v);
        else if(tmp->isitenum()) parameters.emplace_back(checkIteNums(tmp));
        else if(tmp->isletnum()) parameters.emplace_back(checkLetNumTerms(tmp));
        else std::cout<<"Error dag node type (in ite operations)! "<<std::endl, assert(false);
    }
    assert(parameters.size()==2);
    #if checkDebug
        std::cout<<"( if ( "<<cond<<" ) then "<<parameters[0]<<" else "<<parameters[1]<<" )"<<std::endl;
    #endif
    if(cond==1) return parameters[0];
    else return parameters[1];
}
Integer checker::checkMathTerms(dagc* dag){
    assert(dag->isnumop());
    std::vector<Integer> parameters;
    for(size_t i=0;i<dag->children.size();i++){
        dagc* tmp = dag->children[i];
        if(tmp->isnumop()) parameters.emplace_back(checkMathTerms(tmp));
        else if(tmp->isvar()) parameters.emplace_back(getValue(tmp));
        else if(tmp->iscnum()) parameters.emplace_back(tmp->v);
        else if(tmp->isitenum()) parameters.emplace_back(checkIteNums(tmp));
        else if(tmp->isletnum()) parameters.emplace_back(checkLetNumTerms(tmp));
        else std::cout<<"Check Error dag node type (in math operations)! "<<std::endl, assert(false);
        
    }
    if(dag->isabs()){
        assert(parameters.size()==1);
        #if checkDebug
            std::cout<<"( abs( "<<parameters[0]<<" )"<<std::endl;
        #endif
        return parameters[0]>0?parameters[0]:-parameters[1];
    }
    else if(dag->ismod()){
        assert(parameters.size()==2);
        #if checkDebug
            std::cout<<"( "<<parameters[0]<<" % "<<parameters[1]<<" )"<<std::endl;
        #endif
        return parameters[0]%parameters[1];
    }
    else if(dag->isdivint()){
        // definition of the functions div and mod:
        // Boute, Raymond T. (April 1992). 
        // The Euclidean definition of the functions div and mod. 
        // ACM Transactions on Programming Languages and Systems (TOPLAS) 
        // ACM Press. 14 (2): 127 - 144. doi:10.1145/128861.128862.
        assert(parameters.size()==2);
        #if checkDebug
            std::cout<<"( "<<parameters[0]<<" / "<<parameters[1]<<" )"<<std::endl;
        #endif
        // debug: 2021.11.08, div
        if(parameters[0] % parameters[1] == 0){
            return parameters[0] / parameters[1];
        }
        else if(parameters[0]<0 && parameters[1]>0){
            // -3 / 2 = -2 ... 1
            // -2 / 3 = -1 ... 1
            return parameters[0] / parameters[1] - 1;
        }
        else if(parameters[0]>0 && parameters[1]<0){
            // 3 / -2 = -1 ... 1
            // 2 / -3 =  0 ... 2
            return parameters[0] / parameters[1];
        }
        else if(parameters[0]<0 && parameters[1]>0){
            // -3 / -2 = 2 ... 1
            // -2 / -3 = 1 ... 1
            return parameters[0] / parameters[1] + 1;
        }
        else{
            return parameters[0] / parameters[1];
        }
    }
    else if(dag->isadd()){
        #if checkDebug
            std::cout<<"( ";
            for(size_t i=0;i<parameters.size()-1;i++){
                std::cout<<parameters[i]<<" + ";
            }
            std::cout<<parameters[parameters.size()-1]<<" )"<<std::endl;
        #endif
        Integer ans = parameters[0];
        for(size_t i=1;i<parameters.size();i++){
            ans += parameters[i];
        }
        return ans;
    }
    else if(dag->ismul()){
        #if checkDebug
            std::cout<<"( ";
            for(size_t i=0;i<parameters.size()-1;i++){
                std::cout<<parameters[i]<<" * ";
            }
            std::cout<<parameters[parameters.size()-1]<<" )"<<std::endl;
        #endif
        Integer ans = parameters[0];
        for(size_t i=1;i<parameters.size();i++){
            ans *= parameters[i];
        }
        return ans;
    }
    else{ std::cout<<"Check Error math operatios! "<<std::endl; assert(false); }
    return 0;
}
