/* blaster_transformer.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_transformer.hpp"
#include "solvers/blaster/blaster_bits.hpp"
#include <algorithm>

using namespace ismt;

#define doDebug 0
#define decDebug 0

// New: 2021-11-17, transformer has simplifier power.
int blaster_transformer::transform(dagc* root){
    return doAtoms(root);
}

bool blaster_transformer::isFree(Integer lower, bool a_open, Integer upper, bool b_open){
    // [-15, 15] is an free interval via 5 bits.
    if(!a_open && !b_open && lower < 0 && upper > 0){
        int l_len = getBitLength(-lower+1);
        if(l_len==-1) return false;
        int r_len = getBitLength(upper + 1);
        if(r_len==-1) return false;
        if(l_len != r_len) return false;
        return true;
    }
    return false;
}
int blaster_transformer::split(Integer lower, bool a_open, Integer upper, bool b_open){
    if(!a_open && !b_open){
        // x in [0, 6] -> x = 3 + t, where t in [-3, 3].
        return getBitLength(upper - lower + 2) - 1;
    }
    else if(a_open && b_open){
        // x in (-1, 7) -> x in [0, 6] -> x = 3 + t, where t in [-3, 3].
        return getBitLength(upper - lower - 2) - 1;
    }
    else{
        // x in (-1, 6] -> x in [0, 6] -> x = 3 + t, where t in [-3, 3].
        // x in [0, 7) -> x in [0, 6] -> x = 3 + t, where t in [-3, 3].
        return getBitLength(upper - lower + 1) - 1;
    }
}

int blaster_transformer::split_middle(Integer lower, bool a_open, Integer upper, bool b_open){
    // - 1 for half the interval
    if(!a_open && !b_open){
        // x in [0, 6] -> x = 3 + t, where t in [-3, 3].
        return getBitLength(upper - lower + 2) - 1;
    }
    else if(a_open && b_open){
        // x in (-1, 7) -> x in [0, 6] -> x = 3 + t, where t in [-3, 3].
        return getBitLength(upper - lower - 2) - 1;
    }
    else{
        // x in (-1, 6] -> x in [0, 6] -> x = 3 + t, where t in [-3, 3].
        // x in [0, 7) -> x in [0, 6] -> x = 3 + t, where t in [-3, 3].
        return getBitLength(upper - lower + 1) - 1;
    }
}
int blaster_transformer::split_lower(Integer lower, bool a_open, Integer upper, bool b_open){
    if(!a_open && !b_open){
        // x in [0, 7] -> x = 0 + t, where t in [0, 7].
        return getBitLength(upper - lower + 1);
    }
    else if(a_open && b_open){
        // x in (-1, 8) -> x in [0, 7] -> x = 0 + t, where t in [0, 7].
        return getBitLength(upper - lower - 1);
    }
    else{
        // x in (-1, 7] -> x in [0, 7] -> x = 0 + t, where t in [0, 7].
        // x in [0, 9) -> x in [0, 7] -> x = 0 + t, where t in [0, 7].
        return getBitLength(upper - lower);
    }
}

void blaster_transformer::_declare(dagc* root){
    assert(root->isvbool());
    declareBool(root);
}
void blaster_transformer::_declare(dagc* root, unsigned bit){
    bvar res = nullptr;
    if(VarMap.find(root)!=VarMap.end()){
        res = VarMap[root];
        // if(res->isConstant()) return;
        // else if(!res->isClean()) res->clear();
        if(!res->isClean()) res->clear();
    }
    assert(res == nullptr || res->isClean());
    
    assert(bit > 0);
    if(res == nullptr){
        res = solver->mkVar(root->name, bit);
        VarMap.insert(std::pair<dagc*, bvar>(root, res));
    }
    else{
        // res->isClean() then in VarMap
        bvar t = solver->mkInnerVar(root->name+"_tmp", bit);
        solver->copy(t, root->name, res);

    }

    #if decDebug
        std::cout<<"declare variable( "<<root->name<<" ) with bit-width ( "<<bit<<" bits ).\n";
    #endif

    res->setZero();
}
void blaster_transformer::_declare(dagc* root, Integer lower, bool a_open, Integer upper, bool b_open){
    if(lower > upper) assert(false);
    else if(lower == upper && (a_open || b_open)) assert(false);
    // else if(isFree(lower, a_open, upper, b_open)){
    //     // _declare(root, blastBitLength(upper - lower));
    //     _declare(root, blastBitLength(upper - lower + 1));
    // }
    else{
        bvar res = nullptr;

        bool not_in = true;
        if(VarMap.find(root)!=VarMap.end()){
            not_in = false;
            res = VarMap[root];
            // if(res->isConstant()) return;
            // else if(!res->isClean()) res->clear();
            if(!res->isClean()) res->clear();
        }
        assert(res == nullptr || res->isClean());

        // assert( (lower < upper - 1) ||
        //         (lower == upper && !a_open && !b_open) || 
        //         (lower == upper - 1 && (!a_open || !b_open)));
        if(lower == upper && !a_open && !b_open){
            // constant
            solver->copyInt(solver->mkInt(lower), res); 
            res->setName(root->name);
        }
        else{
            // variable
            int bit_lower = split_lower(lower, a_open, upper, b_open);
            int bit_middle = split_middle(lower, a_open, upper, b_open);
            if(bit_middle <= 0){
                int bits = bit_lower<=0?blastBitLength(upper - lower + 1):bit_lower;
                bvar t = solver->mkInnerVar(root->name+"_offset", bits);
                if(lower == 0){
                    solver->copy(t, root->name, res);
                }
                else{
                    solver->copy(solver->Add(t, solver->mkInt(lower)), root->name, res);
                }
                
                solver->Assert(solver->GreaterEqual(t, solver->mkInt(0)));

                if(bit_lower <= 0){
                    if(b_open) solver->Assert(solver->Less(t, solver->mkInt(upper - lower)));
                    else solver->Assert(solver->LessEqual(t, solver->mkInt(upper - lower)));
                }
                #if decDebug
                    std::cout<<"declare variable( "<<root->name<<" ) with lower offset ( "<<root->name<<" = "<<lower<<" + t).\n";
                #endif
            }
            else{
                Integer mid = (lower + upper) / 2;
                bvar t = solver->mkInnerVar(root->name+"_mid_offset", bit_middle);
                solver->copy(solver->Add(t, solver->mkInt(mid)), root->name, res);
                // x in [0, 6] -> x = 3 + t, where t in [-3, 3].
                #if decDebug
                    std::cout<<"declare variable( "<<root->name<<" ) with middle offset ( "<<root->name<<" = "<<mid<<" + t).\n";
                #endif
            }
            
        }

        if(!res->isConstant()){
            if(lower > 0 || upper < 0 || (lower == 0 && a_open) || (upper == 0 && b_open)){
                res->unsetZero();
            }
            else{
                res->setZero();
            }
        }

        if(not_in){
            solver->addVar(res);
            solver->addVarMap(res);
            VarMap.insert(std::pair<dagc*, bvar>(root, res));
        }
    }
}
int blaster_transformer::_assert(dagc* root){
    return transform(root);
}

void blaster_transformer::declareInt(dagc* root){
    if(IntMap.find(root)!=IntMap.end()) return; 
    bvar t = solver->mkInt(root->v);
    IntMap.insert(std::pair<dagc*, bvar>(root, t));
}
bvar blaster_transformer::getInt(dagc* root){
    if(IntMap.find(root)==IntMap.end()){
        declareInt(root);
    } 
    return IntMap[root];
}
bvar blaster_transformer::getLetNum(dagc* root){
    if(NumLetMap.find(root) == NumLetMap.end()){
        bvar ans = doLetNum(root);
        NumLetMap.insert(std::pair<dagc*, bvar>(root, ans));
        return ans;
    }
    else return NumLetMap[root];
}
int blaster_transformer::getLetBool(dagc* root){
    if(BoolLetMap.find(root) == BoolLetMap.end()){
        int ans= -1;
        if(BoolOprMap.find(root->children[0]) != BoolOprMap.end()){
            ans = BoolOprMap[root->children[0]];
        }
        else ans = doAtoms(root->children[0]);
        BoolLetMap.insert(std::pair<dagc*, int>(root, ans));
        return ans;
    }
    else return BoolLetMap[root];
}

void blaster_transformer::declareBool(dagc* root){
    bool_var t = 0;
    if(root->isAssigned()){
        t = (root->v == 1?solver->Lit_True():solver->Lit_False());
    }
    else{
        t = solver->mkBool(root->name);
    }
    BoolMap.insert(std::pair<dagc*, int>(root, t));

}

void blaster_transformer::reset(blaster_solver* s){
    BoolMap.clear();
    BoolLetMap.clear();
    BoolFunMap.clear();
    OprMap.clear();
    BoolOprMap.clear();
    NumLetMap.clear();
    IntFunMap.clear();
    // a new solver
    solver = s;
    VarMap.clear();
    IntMap.clear();
}
void blaster_transformer::reset(){
    BoolMap.clear();
    BoolLetMap.clear();
    BoolFunMap.clear();
    OprMap.clear();
    BoolOprMap.clear();
    NumLetMap.clear();
    IntFunMap.clear();
    auto it = VarMap.begin();
    while(it != VarMap.end()){
        it->second->clear();
        ++it;
    }
}
// ------------do operations------------
bool SortingAddition(const bvar& a, const bvar& b){
    return a->size() < b->size();
}
bvar blaster_transformer::doMathTerms(dagc* dag){
    assert(dag->isnumop());
    if(OprMap.find(dag)!=OprMap.end()) return OprMap[dag];

    std::vector<bvar> parameters;
    for(size_t i=0;i<dag->children.size();i++){
        dagc* tmp = dag->children[i];
        if(tmp->isnumop()) parameters.emplace_back(doMathTerms(tmp));
        else if(tmp->isvar()) parameters.emplace_back(VarMap[tmp]);
        else if(tmp->iscnum()) parameters.emplace_back(getInt(tmp));
        else if(tmp->isitenum()) parameters.emplace_back(doIteNums(tmp));
        else if(tmp->isletnum()) parameters.emplace_back(getLetNum(tmp)); // tmp must have been declared, else error.
        else std::cout<<"Error dag node type (in math operations)! "<<std::endl, assert(false);
        // else tmp->print();
    }
    if(dag->isabs()){
        assert(parameters.size()==1);
        #if doDebug
            std::cout<<"abs( "<<parameters[0]->getName()<<" )"<<std::endl;
        #endif
        bvar t = solver->Absolute(parameters[0]);
        OprMap.insert(std::pair<dagc*, bvar>(dag, t));
        return t;
    }
    else if(dag->ismod()){
        assert(parameters.size()==2);
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName()<<" mod "<<parameters[1]->getName()<<" )"<<std::endl;
        #endif
        bvar t = solver->Modulo(parameters[0], parameters[1]);
        OprMap.insert(std::pair<dagc*, bvar>(dag, t));
        return t;
    }
    else if(dag->isdivint()){
        assert(parameters.size()==2);
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName()<<" / "<<parameters[1]->getName()<<" )"<<std::endl;
        #endif
        bvar t = solver->Divide(parameters[0], parameters[1]);
        OprMap.insert(std::pair<dagc*, bvar>(dag, t));
        return t;
    }
    else if(dag->isadd()){
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName();
            for(size_t i=1;i<parameters.size();i++){
                std::cout<<" + "<<parameters[i]->getName();
            }
            std::cout<<" )"<<std::endl;
        #endif
        // sort addition
        bvar ans = nullptr;
        while(parameters.size() > 1){
            if(GA) std::sort(parameters.begin(), parameters.end(), SortingAddition);
            ans = solver->Add(parameters[0], parameters[1]);
            parameters.erase(parameters.begin());
            parameters.erase(parameters.begin());
            parameters.emplace_back(ans);
        }
        bvar t = parameters[0];
        OprMap.insert(std::pair<dagc*, bvar>(dag, t));
        return t;
    }
    else if(dag->ismul()){
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName();
            for(size_t i=1;i<parameters.size();i++){
                std::cout<<" * "<<parameters[i]->getName();
            }
            std::cout<<" )"<<std::endl;
        #endif
        bvar ans = parameters[0];
        for(size_t i=1;i<parameters.size();i++){
            ans = solver->Multiply(ans, parameters[i]);
        }
        bvar t = ans;
        OprMap.insert(std::pair<dagc*, bvar>(dag, t));
        return t;
    }
    else std::cout<<"Error math operatios! "<<std::endl, assert(false);
}

bvar blaster_transformer::doIteNums(dagc* dag){
    assert(dag->isitenum());
    if(OprMap.find(dag)!=OprMap.end()) return OprMap[dag]; // fast finding.
    int cond = solver->Lit_False();
    assert(dag->children[0]->isbool()||dag->children[0]->iscbool());
    if(dag->children[0]->iscbool()){
        cond = dag->children[0]->istrue()?solver->Lit_True():solver->Lit_False();
    }
    else{
        if(dag->children[0]->isboolop()){
            // debug: 2021.08.22, cond may be a nested formula.
            // TODO.
            cond = doAtoms(dag->children[0]);
        }
        else if(dag->children[0]->isletbool()){
            cond = getLetBool(dag->children[0]);
        }
        else{
            assert(dag->children[0]->isvbool());
            cond = BoolMap[dag->children[0]];
        }
    }
    std::vector<bvar> parameters;
    for(size_t i=1;i<dag->children.size();i++){
        dagc* tmp = dag->children[i];
        if(tmp->isnumop()) parameters.emplace_back(doMathTerms(tmp));
        else if(tmp->isvar()) parameters.emplace_back(VarMap[tmp]);
        else if(tmp->iscnum()) parameters.emplace_back(getInt(tmp));
        else if(tmp->isitenum()) parameters.emplace_back(doIteNums(tmp));
        else if(tmp->isletnum()) parameters.emplace_back(getLetNum(tmp)); // tmp must have been declared, else error.
        else std::cout<<"Error dag node type (in ite operations)! "<<std::endl, assert(false);
        // else tmp->print();
    }
    assert(parameters.size()==2);
    bvar t = solver->Ite_num(cond, parameters[0], parameters[1]);
    OprMap.insert(std::pair<dagc*, bvar>(dag, t));
    return t;
}

int blaster_transformer::doMathAtoms(dagc* dag){
    assert(dag->iscomp());
    std::vector<bvar> parameters;
    for(size_t i=0;i<dag->children.size();i++){
        dagc* tmp = dag->children[i];
        if(tmp->isnumop()) parameters.emplace_back(doMathTerms(tmp));
        else if(tmp->isvar()) parameters.emplace_back(VarMap[tmp]);
        else if(tmp->iscnum()) parameters.emplace_back(getInt(tmp));
        else if(tmp->isitenum()) parameters.emplace_back(doIteNums(tmp));
        else if(tmp->isletnum()) parameters.emplace_back(getLetNum(tmp)); // tmp must have been declared, else error.
        else{ tmp->print(); std::cout<<"Error dag node type (in math atoms)! "<<std::endl; assert(false); }
    }
    assert(parameters.size()==2);
    // 2012-11-17: new simplifier for operations by using variables' intervals.
    if(dag->iseq()){
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName()<<" == "<<parameters[1]->getName()<<" )"<<std::endl;
        #endif
        return solver->Equal(parameters[0], parameters[1]);
    }
    else if(dag->isneq()){
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName()<<" != "<<parameters[1]->getName()<<" )"<<std::endl;
        #endif
        return solver->NotEqual(parameters[0], parameters[1]);
    }
    else if(dag->isle()){ // x <= y
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName()<<" <= "<<parameters[1]->getName()<<" )"<<std::endl;
        #endif
        return solver->LessEqual(parameters[0], parameters[1]);
    }
    else if(dag->islt()){ // x < y
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName()<<" < "<<parameters[1]->getName()<<" )"<<std::endl;
        #endif
        return solver->Less(parameters[0], parameters[1]);
    }
    else if(dag->isge()){ // x >= y
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName()<<" >= "<<parameters[1]->getName()<<" )"<<std::endl;
        #endif
        return solver->GreaterEqual(parameters[0], parameters[1]);
    }
    else if(dag->isgt()){
        #if doDebug
            std::cout<<"( "<<parameters[0]->getName()<<" > "<<parameters[1]->getName()<<" )"<<std::endl;
        #endif
        return solver->Greater(parameters[0], parameters[1]);
    }
    else std::cout<<"Error math atom operatios! "<<std::endl, assert(false);
}

int blaster_transformer::doAtoms(dagc* dag){
    // if(BoolOprMap.find(dag)!=BoolOprMap.end()) return BoolOprMap[dag]; // fast finding.
    if(dag->isand()||dag->isor()||dag->isnot()||dag->isitebool()||dag->iseqbool()||dag->isneqbool()){
        std::vector<int> parameters;
        for(size_t i=0;i<dag->children.size();i++){
            dagc* tmp = dag->children[i];
            if(tmp->isboolop()) parameters.emplace_back(doAtoms(tmp));
            else if(tmp->isnumop()) parameters.emplace_back(doMathAtoms(tmp));
            else if(tmp->isvbool()) parameters.emplace_back(BoolMap[tmp]);
            else if(tmp->iscbool()) parameters.emplace_back(tmp->v==1?solver->Lit_True():solver->Lit_False()); // 1 for true, 0 for false.
            else if(tmp->isitebool()) parameters.emplace_back(doAtoms(tmp));
            else if(tmp->isletbool()) parameters.emplace_back(getLetBool(tmp));  // tmp must have been declared, else error.
            else if(tmp->islet()) parameters.emplace_back(doLetAtoms(tmp));
            else if(tmp->iseqbool()) parameters.emplace_back(doAtoms(tmp));
            else if(tmp->isneqbool()) parameters.emplace_back(doAtoms(tmp));
            else{ tmp->print(); std::cout<<"Error dag node type (in logic atoms)! "<<std::endl, assert(false);}
            // 2012-11-17: new simplifier for and/or.
            int prop = parameters.back();
            // and/or has property of short circuit.
            if(dag->isand()){
                for(size_t i=0;i<parameters.size();i++){
                    if(-prop == parameters[i]) return solver->Lit_False();
                }
                if(prop==solver->Lit_False()) return solver->Lit_False();
                else if(prop==-solver->Lit_True()) return solver->Lit_False();
            }
            else if(dag->isor()){
                for(size_t i=0;i<parameters.size();i++){
                    if(-prop == parameters[i]) return solver->Lit_True();
                }
                if(prop==-solver->Lit_False()) return solver->Lit_True();
                else if(prop==solver->Lit_True()) return solver->Lit_True();
            }
        }
        if(dag->isand()){
            #if doDebug
                std::cout<<"( And( ";
                for(size_t i=0;i<parameters.size()-1;i++){
                    std::cout<<parameters[i]<<" , ";
                }
                std::cout<<parameters[parameters.size()-1]<<" ) )"<<std::endl;
            #endif
            int ans = solver->And(parameters);
            BoolOprMap.insert(std::pair<dagc*, int>(dag, ans));
            return ans;
        }
        else if(dag->isor()){
            #if doDebug
                std::cout<<"( Or( ";
                for(size_t i=0;i<parameters.size()-1;i++){
                    std::cout<<parameters[i]<<" , ";
                }
                std::cout<<parameters[parameters.size()-1]<<" ) )"<<std::endl;
            #endif
            int ans = solver->Or(parameters);
            BoolOprMap.insert(std::pair<dagc*, int>(dag, ans));
            return ans;
        }
        else if(dag->isnot()){
            assert(parameters.size()==1);
            #if doDebug
                std::cout<<"( Not( "<<parameters[0]<<" ) )"<<std::endl;
            #endif
            int ans = solver->Not(parameters[0]);
            BoolOprMap.insert(std::pair<dagc*, int>(dag, ans));
            return ans;
        }
        else if(dag->isitebool()){
            // new simplifier for itebool: once ite(c, x, y) with x == y => ite(c, x, y) == x
            assert(parameters.size()==3);
            #if doDebug
                std::cout<<"( if ( "<<parameters[0]<<" ) then "<<parameters[1]<<" else "<<parameters[2]<<" )"<<std::endl;
            #endif
            if(parameters[1]==parameters[2]) return parameters[1];
            int ans = solver->Ite_bool(parameters[0], parameters[1], parameters[2]);
            BoolOprMap.insert(std::pair<dagc*, int>(dag, ans));
            return ans;
        }
        else if(dag->iseqbool()){
            assert(parameters.size()==2);
            #if doDebug
                std::cout<<"( ( "<<parameters[0]<<" ) ==b ( "<<parameters[1]<<" ) )"<<std::endl;
            #endif
            if(parameters[0]==parameters[1]) return solver->Lit_True();
            int ans = solver->EqualBool(parameters[0], parameters[1]);
            BoolOprMap.insert(std::pair<dagc*, int>(dag, ans));
            return ans;
        }
        else if(dag->isneqbool()){
            // new simplifier for itebool: once ite(c, x, y) with x == y => ite(c, x, y) == x
            assert(parameters.size()==2);
            #if doDebug
                std::cout<<"( ( "<<parameters[0]<<" ) != ( "<<parameters[1]<<" ) )"<<std::endl;
            #endif
            int ans = solver->NotEqualBool(parameters[0], parameters[1]);
            BoolOprMap.insert(std::pair<dagc*, int>(dag, ans));
            return ans;
        }
        else std::cout<<"Error logic operatios (in logic atoms)! "<<std::endl, assert(false);
    }
    else if(dag->iscomp()){
        int ans = doMathAtoms(dag);
        BoolOprMap.insert(std::pair<dagc*, int>(dag, ans));
        return ans;
    }
    else if(dag->iscbool()){
        #if doDebug
            std::cout<<"( "<<(dag->v==1?("true"):("false"))<<" )"<<std::endl;
        #endif
        return dag->v==1?solver->Lit_True():solver->Lit_False();
    }
    else if(dag->isvbool()){
        // debug: 2021.08.29, dag->isvbool().
        #if doDebug
            std::cout<<"( "<<dag->name<<" )"<<std::endl;
        #endif
        return BoolMap[dag];
    }
    else if(dag->islet()){
        // only use one times, so no need to add to map
        return doLetAtoms(dag);
    }
    else if(dag->isletbool()){
        // this node must have been set.
        int ans = getLetBool(dag);
        BoolOprMap.insert(std::pair<dagc*, int>(dag, ans));
        return ans;
    }
    else{ dag->print(); std::cout<<"Error logic operatios! "<<std::endl, assert(false); }
}

bvar blaster_transformer::doLetNum(dagc* dag){
    assert(dag->isletnum());
    // use its children so that no need to add to map.(its children has added.)
    dagc* x = dag->children[0];
    if(x->isnumop())
        return doMathTerms(x);
    else if(x->isitenum())
        return doIteNums(x);
    else if(x->isvar())
        return VarMap[x];
    else if(x->iscnum())
        return getInt(x);
    else if(x->isletnum())
        return getLetNum(x);
    else std::cout<<"Error let type (in let operations)! "<<std::endl, assert(false);

    // else dag->print(), assert(false);
}

// doLetAtoms will never invoked.
int blaster_transformer::doLetAtoms(dagc* dag){
    assert(dag->islet());
    std::vector<dagc* > key_list; 
    
    // dag->children[0] is the atom.
	dagc* formula = dag;
    while(formula->islet()){
        // make local variables to map.
        for(size_t i=1;i<formula->children.size();i++){
            dagc* child = formula->children[i];
            key_list.emplace_back(child);
            if(child->isletbool()){
                int ans = doAtoms(child->children[0]);
                BoolLetMap.insert(std::pair<dagc*, int>(child, ans));
            }
            else if(child->isletnum()){
                bvar ans = doLetNum(child);
                NumLetMap.insert(std::pair<dagc*, bvar>(child, ans));
            }
            else std::cout<<"Error let variable declaration! "<<std::endl, assert(false);
        }
        formula = formula->children[0];
    }
    int answer = doAtoms(formula);

    // clear variable map. for they are local variables.
    while (key_list.size() > 0) {
        if(key_list.back()->isletbool()) BoolLetMap.erase(key_list.back());
        else NumLetMap.erase(key_list.back());
        key_list.pop_back();
    }

    return answer;
}