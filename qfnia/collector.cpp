/* collector.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "qfnia/collector.hpp"
#include "solvers/blaster/blaster_bits.hpp"
#include <queue>

using namespace ismt;

#define colDebug 0
#define cntDebug 0

// remove unconstrained vairables
bool Collector::removeVars(){
    // add to model?
    std::vector<dagc*>::iterator it = data->vbool_list.begin();
    while(it!=data->vbool_list.end()){
        if(usedTimes(*it) == 0 ){
            // if(!model->has(*it)) model->set(*it, 0); // can not set value, because of substiting
            unused_vars.insert(*it);
            // it = data->vbool_list.erase(it);
        }
        ++it;
    }

    it = data->vnum_int_list.begin();
    while(it!=data->vnum_int_list.end()){
        if(usedTimes(*it) == 0){
            // if(!model->has(*it)) model->set(*it, 0);
            unused_vars.insert(*it);
            // it = data->vnum_int_list.erase(it);
        }
        ++it;
    }
    // it = data->vnum_real_list.begin();
    // while(it!=data->vnum_real_list.end()){
    //     if(usedTimes(*it) == 0 ){
    //         dagc* x = *it;
    //         unused_vars.insert(x);
    //         // it = data->vnum_real_list.erase(it);
    //     }
    //     ++it;
    // }

    return true;
}

// collect must, may and neq nodes.
bool Collector::collect(dagc* root, bool must){
    if(root->isvar()){ ++used[root]; }
    else if(root->iscbool() || root->iscnum()){}
    else if(root->isnumop() || root->isitenum() || root->isletvar() ||
            root->isnot()||root->iseqbool()||root->isneqbool() || root->isand()){
        for(unsigned i=0;i<root->children.size();i++){
            collect(root->children[i], must);
        }
    }
    else if(root->iscomp()){
        // if(!must) mayBNodes.emplace_back(t);
        if(root->isneq()) collectNeq(root, must);
        else collectMax(root);
        for(unsigned i=0;i<root->children.size();i++){
            collect(root->children[i], must);
        }
    }
    else{
        if(root->isor() || root->isitebool()){
            for(unsigned i=0;i<root->children.size();i++){
                collect(root->children[i], false);
            }
        }
        else if(root->islet()){
            dagc* x = root->children[0];
            while(x->islet()){
                x = x->children[0];
            }
            
            collect(x, must);
        }
        else{
            std::cout<<"collect error!"<<std::endl; 
            root->print(); assert(false); 
        }
    }
    
    return true;
}
void Collector::collectNeq(dagc* root, bool must){
    // because of nnf, not is only header of bool literal.
    assert(root->isneq());
    dagc* a = root->children[0];
    dagc* b = root->children[1];
    // a != b
    if(a->isvnum() && b->isvnum()){
        if(must){
            mustNeqGraph.addEdge(a->name, b->name);
            mustNeqGraph.addEdge(b->name, a->name);
        } 
        MayNeqGraph.addEdge(a->name, b->name);
        MayNeqGraph.addEdge(b->name, a->name);
    }
}

int Collector::count_node(dagc* root){
    if(root->children.size() == 0) return 0;
    else{
        int ans = 0;
        for(unsigned i=0;i<root->children.size();i++){
            ans += 1;
            ans += count_node(root->children[i]);
        }
        return ans;
    }
}
void Collector::collectMaxCMul(dagc* root, boost::unordered_set<dagc*>& vars, Integer& maximum){
    if(root->isvnum()){
        vars.insert(root);
    }
    else if(root->iscnum()){
        Integer tmp = (root->v>0?root->v:-root->v);
        if(tmp > maximum){
            maximum = tmp;
        }
    }
    else{
        for(unsigned i=0;i<root->children.size();i++){
            collectMaxCMul(root->children[i], vars, maximum);
        }
    }
}

void Collector::collectMaxC(dagc* root, boost::unordered_map<dagc*, Integer>& dimap){
    if(root->isvar()){
        if(dimap.find(root)==dimap.end()){
            dimap.insert(std::pair<dagc*, Integer>(root, 0));
        }
    }
    else if(root->ismul()){
        boost::unordered_set<dagc*> vars;
        Integer maximum = 0;
        collectMaxCMul(root, vars, maximum);
        auto it = vars.begin();
        while(it != vars.end()){
            if(dimap.find(*it) != dimap.end()){
                if(dimap[*it] < maximum){
                    dimap[*it] = maximum;
                }
            }
            else{
                dimap.insert(std::pair<dagc*, Integer>(*it, maximum));
            }
            ++it;
        }
    }
    else{
        for(unsigned i=0;i<root->children.size();i++){
            collectMaxC(root->children[i], dimap);
        }
    }
}
void Collector::collectMax(dagc* root){
    if(!root->iseq()) return;
    // collect vars and mul
    boost::unordered_map<dagc*, Integer> dimap;
    boost::unordered_map<dagc*, Integer> lef_map;
    boost::unordered_map<dagc*, Integer> rig_map;
    collectMaxC(root->children[0], lef_map); // left
    collectMaxC(root->children[1], rig_map); // right
    Integer lef_maximum = 0;
    dagc* lef_maxcvar = nullptr;
    auto it = lef_map.begin();
    while(it != lef_map.end()){
        if(it->second > lef_maximum){
            lef_maximum = it->second;
            lef_maxcvar = it->first;
        }
        dimap.insert(std::pair<dagc*, Integer>(it->first, it->second));
        ++it;
    }
    Integer rig_maximum = 0;
    dagc* rig_maxcvar = nullptr;
    it = rig_map.begin();
    while(it != rig_map.end()){
        if(it->second > rig_maximum){
            rig_maximum = it->second;
            rig_maxcvar = it->first;
        }
        dimap.insert(std::pair<dagc*, Integer>(it->first, it->second));
        ++it;
    }

    // reverse the dimap

    //  1. set dimap
    Integer maximum = rig_maximum - lef_maximum;
    // data->printAST(root);
    // std::cout<<std::endl;
    // std::cout<<"count node: "<<count_node(root)<<std::endl;
    // std::cout<<rig_maximum<<" "<<lef_maximum<<" "<<maximum<<std::endl;
    // std::cout<<"----------------------------------"<<std::endl;
    maximum = maximum > 0 ? maximum : -maximum;

    //  2. reverse the map
    it = dimap.begin();
    while(it != dimap.end()){
        dagc* var = it->first;
        if(var != lef_maxcvar && var != rig_maxcvar){
            if(MaxNumber.find(var) != MaxNumber.end()){
                Integer val = MaxNumber[var];
                if(val <= maximum){
                    if(val!=0){
                        MaxNumber[var] = maximum / val > val? maximum / val : val;
                    }
                    else{
                        MaxNumber[var] = maximum;
                    }
                }
            }
            else MaxNumber.insert(std::pair<dagc*, Integer>(var, maximum));
        }
        ++it;
    }
}
int Collector::get_max_number_count(){
    int cnt = 0;
    auto it = MaxNumber.begin();
    while(it != MaxNumber.end()){
        if(it->second > 1) ++cnt;
        ++it;
    }
    return cnt;
}

void Collector::collect(){
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        collect(*it);
        ++it;
    }
}

unsigned int Collector::usedTimes(dagc* var){
    return used[var];
}

// analyze each comp constraint and check bounds.
void Collector::addInterval(dagc* var, bool must, const Interval& interval, bool neq){
    feasible_set* fs = nullptr;
    if(must){
        if(variables.find(var)!=variables.end()){
            fs = variables[var];
            if(neq){
                fs->deleteInterval(interval);
            }
            else{
                fs->intersectInterval(interval);
                if(fs->isSetEmpty()){
                    state = State::UNSAT;
                }
            }
        }
        else{
            if(neq){
                fs = new feasible_set(FullInterval);
                fs->deleteInterval(interval);
            }
            else{
                fs = new feasible_set(interval);
            }
            variables.insert(std::pair<dagc*, feasible_set*>(var, fs));
        }
    }
    else{
        // may
        if(mvariables.find(var)!=mvariables.end()){
            fs = mvariables[var];
            if(neq){
                fs->deleteInterval(interval);
            }
            else{
                fs->addInterval(interval);
            }
        }
        else{
            if(neq){
                fs = new feasible_set(FullInterval);
                fs->deleteInterval(interval);
            }
            else{
                fs = new feasible_set(interval);
            }
            mvariables.insert(std::pair<dagc*, feasible_set*>(var, fs));
        }
    }
}

bool Collector::setBound(dagc* d, dagc* var, Integer ans, bool must){
    // assign the sat asssertion
    if(must && !d->isneq()){
        d->assign(1);
    }
    if(d->isneq()){
        addInterval(var, must, Interval(to_Value(ans), false, to_Value(ans), false), true);
    }
    else if(d->iseq()){
        addInterval(var, must, Interval(to_Value(ans), false, to_Value(ans), false));
    }
    else{ // d->isle(), d->islt(), d->isge(), d->isgt()
        if(d->isle()){
            // (<= x 6) <-> x <= 6
            addInterval(var, must, Interval(Value::minus_infty(), true, to_Value(ans), false));
        }
        else if(d->islt()){
            // (< x 6) -> x < 6
            addInterval(var, must, Interval(Value::minus_infty(), true, to_Value(ans - 1), false));
        }
        else if(d->isge()){
            // (>= x 6) -> x >= 6
            addInterval(var, must, Interval(to_Value(ans), false, Value::plus_infty(), true));
        }
        else{ // d->isgt()
            // (> x 6) -> x > 6
            addInterval(var, must, Interval(to_Value(ans + 1), false, Value::plus_infty(), true));
        }
    }
    return true;
}

bool Collector::doIntervalLit(dagc* root, bool isTop){
    if(root->isor()){
        for(size_t i=0;i<root->children.size();i++){
            assert(root->children[i]->iscomp());
            dagc* x = root->children[i];
            dagc* a = x->children[0];
            dagc* b = x->children[1];
            if(a->isvar() && b->iscnum()){
                if(!setBound(root, a, b->v, isTop)){
                    return false;
                }
            }
            else if(b->isvar() && a->iscnum()){
                root->CompRev();
                if(!setBound(root, b, a->v, isTop)){
                    root->CompRev();
                    return false;
                }
                else{ root->CompRev(); }
            }
            else assert(false);
        }
    }
    else{
        assert(root->iscomp());
        dagc* a = root->children[0];
        dagc* b = root->children[1];
        if(a->isvar() && b->iscnum()){
            if(!setBound(root, a, b->v, isTop)){
                return false;
            }
        }
        else if(b->isvar() && a->iscnum()){
            root->CompRev();
            if(!setBound(root, b, a->v, isTop)){
                root->CompRev();
                return false;
            }
            else{ root->CompRev(); }
        }
        else assert(false);
    }
    return true;
}

bool isEasyIte(dagc* root){
    if( root->isitenum() && 
        root->children[0]->iscnum() &&
        root->children[1]->iscnum()) return true;
    else return false;
}

bool Collector::isSimpleComp(dagc* root){
    // x != y -> x - y != 0 which it is more difficult.
    // x = y -> x - y = 0 which it is more difficult.
    dagc* x = root->children[0];
    dagc* y = root->children[1];

    return (
        (x->isvar() && y->iscnum()) ||
        (x->iscnum() && y->isvar()) 
    ); // mul and var for future.
}

int Collector::isOrInterval(dagc* root){
    // (or (= s1 1) (= s1 4) (= s1 8)) => s1 [1,1],[4,4],[8,8]
    if(!root->isor()) return false;
    boost::unordered_set<dagc*> vars;
    for(unsigned i=0;i<root->children.size();i++){
        dagc* r = root->children[i];
        if(r->iscomp() && isSimpleComp(r)){
            if(r->children[0]->isvnum()) vars.insert(r->children[0]);
            else vars.insert(r->children[1]);
        }
        else return 0;
    }

    return vars.size();
}
void Collector::doOrInterval(dagc* root){
    // (or (= s1 1) (= s1 4) (= s1 8)) => s1 [1,1],[4,4],[8,8]
    dagc* var = nullptr;
    for(unsigned i=0;i<root->children.size();i++){
        dagc* r = root->children[i];
        dagc* a = r->children[0];
        dagc* b = r->children[1];
        if(a->isvar() && b->iscnum()){
            var = a;
            if(!setBound(r, a, b->v, false)){
                state = State::UNSAT;
                return;
            }
        }
        else if(b->isvar() && a->iscnum()){
            var = b;
            r->CompRev();
            if(!setBound(r, b, a->v, false)){
                r->CompRev();
                state = State::UNSAT;
                return;
            }
            else r->CompRev();
        }
    }

    // set in may bound and then assign to must bound
    feasible_set* res = mvariables[var];
    feasible_set* fs = nullptr;
    if(variables.find(var)!=variables.end()){
        fs = variables[var];
        fs->intersectFeasibleSet(*res);
        if(fs->isSetEmpty()){
            state = State::UNSAT;
        }
    }
    else{
        fs = new feasible_set();
        fs->addFeasibleSet(*res);
        variables.insert(std::pair<dagc*, feasible_set*>(var, fs));
    }
}
bool Collector::analyze(dagc* root, bool isTop){
    if(root->iscomp()){
        dagc* a = root->children[0];
        dagc* b = root->children[1];
        if(a->isvar() && b->iscnum()){
            return setBound(root, a, b->v, isTop);
        }
        else if(b->isvar() && a->iscnum()){
            root->CompRev();
            bool ans = setBound(root, b, a->v, isTop);
            root->CompRev();
            return ans;
        }

        // else if(a->isvar() && isEasyIte(b)){
        //     return setBound(root, a, b->v, isTop);
        // }
        // else if(b->isvar() && isEasyIte(a) ){
        //     root->CompRev();
        //     bool ans = setBound(root, b, a->v, isTop);
        //     root->CompRev();
        //     return ans;
        // }
    }
    else if(root->isInterval()){
        // the answer interval of univariate polynomial
        return doIntervalLit(root, isTop);
    }
    else if(root->isnumop() || root->isconst() || root->isvar() || root->iseqbool() || root->isneqbool()){}
    else{
        // nnf 
        assert(!root->isnot() || (root->isnot() && !root->children[0]->iscomp()));
        
        if(root->isor()){
            int ans = isOrInterval(root);
            if(ans == 1 && isTop){
                // (or (= s1 1) (= s1 4) (= s1 8)) => s1 [1,1],[4,4],[8,8]
                doOrInterval(root);
                return true;
            }
        }

        isTop = isTop & root->isand();
        for(size_t i=0;i<root->children.size();i++){
            analyze(root->children[i], isTop);
        }
    }
    return true;
}

bool Collector::analyze(){
    // x >= 0 and x >= 1 -> x >= 1
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        (*it)->unsetTop();
        analyze(*it);
        ++it;
    }
    
    // remove sat assertions and mark top
    it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        if((*it)->isAssigned()){
            if((*it)->istrue()){
                it = data->assert_list.erase(it);
            }
            else{
                return false; // (*it)->isfalse()
            }
        }
        else{
            (*it)->setTop();
            ++it;
        }
    }

    return state!=State::UNSAT;
}
bool Collector::check(){
    return true;
}

int Collector::count_var(dagc* root){
    int ans = 0;
    if(root->isvnum()){
        ans += 1;
        if(mul_var_count.find(root) != mul_var_count.end()){
            mul_var_count[root] += 1;
        }
        else{
            mul_var_count.insert(std::pair<dagc*, int>(root, 1));
        }
    }
    else{
        for(unsigned i=0;i<root->children.size();i++){
            ans += count_var(root->children[i]);
        }
    }
    return ans;
}
int Collector::count_mul(dagc* root){
    if(root->children.size() == 0) return 0;
    else{
        int ans = 0;
        if(root->ismul()){
            // if(count_var(root) > 1){
            //     // only non-linear mul is counted
            //     ans += 1;
            //     for(unsigned i=0;i<root->children.size();i++){
            //         ans += count_mul(root->children[i]);
            //     }
            // }
            
            // var * var -> weight 2
            // var * c   -> weight 1
            // if(count_var(root) > 1) count_var(root);
            count_var(root);
            ans += 1;
            for(unsigned i=0;i<root->children.size();i++){
                ans += count_mul(root->children[i]);
            }
        }
        else{
            for(unsigned i=0;i<root->children.size();i++){
                ans += count_mul(root->children[i]);
            }
        }
        return ans;
    }
}
int Collector::count_add(dagc* root){
    if(root->children.size() == 0) return 0;
    else{
        int ans = 0;
        if(root->isadd()){
            ans += 1;
            for(unsigned i=0;i<root->children.size();i++){
                ans += count_add(root->children[i]);
            }
        }
        else{
            for(unsigned i=0;i<root->children.size();i++){
                ans += count_add(root->children[i]);
            }
        }
        return ans;
    }
}
int Collector::count_mul(){
    int ans = 0;
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        ans += count_mul(*it);
        ++it;
    }
    return ans;
}
int Collector::count_add(){
    int ans = 0;
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        ans += count_add(*it);
        ++it;
    }
    return ans;
}
int Collector::count_node(){
    int ans = 0;
    auto it = data->assert_list.begin();
    while(it!=data->assert_list.end()){
        ans += count_node(*it);
        ++it;
    }
    return ans;
}

bool Collector::set_model(){
    auto it = variables.begin();
    while(it != variables.end()){
        if(it->second->isPoint()){
            dagc* x = it->first;
            Integer val = it->second->getPoint();
            model->set(x, val);
            it = variables.erase(it++);
        }
        else ++it;
    }
    
    auto uit = unused_vars.begin();
    while(uit != unused_vars.end()){
        dagc* x = *uit;
        if(x->isAssigned()){}
        else{
            if(!model->isSub(x)){
                // not a substituted variable, auto set a value
                if(x->isvbool()){
                    model->set(x, 0);
                }
                else if(x->isvnum()){
                    if(variables.find(x) != variables.end()){
                        // have a feasible set
                        feasible_set* fs = variables[x];
                        model->set(x, fs->pick_value());
                    }
                    else{
                        // without a feasible set
                        model->set(x, 0);
                    }
                }
            }
            else{
                // check whether all substituted variables are not used.
                std::vector<dagc*> svars;
                bool all_flag = true;
                model->get_sub_set(x, svars);
                // 2023-05-23: all substituted variables are unused.
                for(unsigned i=0;i<svars.size();i++){
                    if( unused_vars.find(svars[i]) == unused_vars.end()){
                        all_flag = false;
                        break;
                    }
                }
                if(all_flag){
                    // 2023-05-23: use subRoot instead, or it will make error.
                    //             because the feasible intervals of subRoot are updated only !!!
                    x = model->getSubRoot(x);
                    if(variables.find(x) != variables.end()){
                        // have a feasible set
                        feasible_set* fs = variables[x];
                        
                        model->set(x, fs->pick_value());
                    }
                    else{
                        // without a feasible set
                        model->set(x, 0);
                    }
                }
            }

        }

        ++uit;
    }

    return true;
}
bool Collector::rewrite(){
    #if colDebug
        // 3. analyze each comp constraint and check bounds.
        data->print_constraints();
        std::cout<<"\nanalyzing..."<<std::endl;
    #endif
    bool ans = analyze();
    if(!ans){
        state = State::UNSAT;
        return false;
    }
    // update feasible set for variables
    update_feasible_set();
    #if colDebug
        std::cout<<"\nafter analyzing..."<<std::endl;
        data->print_constraints();
    #endif

    #if colDebug
        // 1. collect must, may bound and neq nodes with counting used variables.
        std::cout<<"\nbound collecting..."<<std::endl;
    #endif
    collect();
    
    #if colDebug
        // 2. simplify unconstrainted variables.
        data->print_constraints();
        std::cout<<"\nremoving unconstrained variables..."<<std::endl;
    #endif
    removeVars();

    ans &= set_model();
    if(!ans){
        state = State::UNSAT;
        return false;
    }
    
    ans &= check();
    if(!ans){
        state = State::UNSAT;
        return false;
    }

    #if cntDebug
        int n = count_node();
        int a = count_add();
        int m = count_mul();
        std::cout<<"Total Nodes: "<<n<<std::endl;
        std::cout<<"Total Add Nodes: "<<a<<std::endl;
        std::cout<<"Total Mul Nodes: "<<m<<std::endl;
    #endif

    // print_feasible_set();

    // auto it = MaxNumber.begin();
    // while(it != MaxNumber.end()){
    //     std::cout<<it->first->name<<" "<<it->second<<std::endl;
    //     ++it;
    // }
    // std::cout<<"must bound"<<std::endl;
    // auto it = variables.begin();
    // while(it!=variables.end()){
    //     std::cout<<it->first->name<<" "<<*(it->second)<<std::endl;
    //     ++it;
    // }
    // std::cout<<"may bound"<<std::endl;
    // it = mvariables.begin();
    // while(it!=mvariables.end()){
    //     std::cout<<it->first->name<<" "<<*(it->second)<<std::endl;
    //     ++it;
    // }
    // mustNeqGraph.print();

    return true;
}
int Collector::get_must_distinct_bit(dagc* var){
    int size = mustNeqGraph.size(var->name);
    if(size == 0) return -1;
    else return blastBitLength(size);
}
int Collector::get_may_distinct_bit(dagc* var){
    int size = MayNeqGraph.size(var->name);
    if(size == 0) return -1;
    else return blastBitLength(size);
}
int Collector::get_max_number_bit(dagc* var){
    if(MaxNumber.find(var) != MaxNumber.end()){
        Integer val = MaxNumber[var];
        if(MaxNumber[var] <= 1) return -1; // no need
        else return blastBitLength(val);
    }
    else return -1;
}
int Collector::get_may_constraint_bit(dagc* var){
    if(mvariables.find(var) != mvariables.end()){
        feasible_set* fs = mvariables[var];
        
        if(fs != nullptr){
            Interval i = fs->get_cover_interval();
            // Interval i = fs->back();
            if(isFull(i)){}
            else if(ninf(i)){
                return blastBitLength(to_long(getUpper(i)) + 1);
            }
            else if(pinf(i)){
                return blastBitLength(to_long(getLower(i)) - 1);
            }
            else{
                // isPoint(i) || bouned
                Integer a = abs(to_long(getLower(i)));
                Integer b = abs(to_long(getUpper(i)));
                
                return blastBitLength( (a>b?a:b) + 1);
            }
        }
    }
    
    return -1;
}

int Collector::get_var_mul_count(dagc* var){
    if(mul_var_count.find(var) == mul_var_count.end()){
        return 0;
    }
    else return mul_var_count[var];
}

void Collector::print_feasible_set(){
    auto it = variables.begin();
    while(it != variables.end()){
        std::vector<Interval> itvs;
        it->second->get_intervals(itvs);

        if(itvs.size() == 1){
            if(!ninf(itvs[0])){
                poly::Integer val = getRealLower(itvs[0]);
                if(val >= 0){
                    std::cout<<"(assert (>= "+it->first->name+" "<<val<<"))\n";
                }
                else{
                    std::cout<<"(assert (>= "+it->first->name+" (- "<<-val<<")))\n";
                }
            }
            if(!pinf(itvs[0])){
                poly::Integer val = getRealUpper(itvs[0]);
                if(val >= 0){
                    std::cout<<"(assert (<= "+it->first->name+" "<<val<<"))\n";
                }
                else{
                    std::cout<<"(assert (<= "+it->first->name+" (- "<<-val<<")))\n";
                }
            }
        }
        else{
            std::string lemma = "";
            
            for(unsigned i=0;i<itvs.size();i++){

                if(!ninf(itvs[i]) && !pinf(itvs[i])){
                    poly::Integer lower = getRealLower(itvs[i]);
                    poly::Integer upper = getRealUpper(itvs[i]);
                    if(lower >= 0)
                        lemma += "(and (>= "+it->first->name+" "+ std::to_string(to_long(lower))+") ";
                    else
                        lemma += "(and (>= "+it->first->name+" (- "+ std::to_string(-to_long(lower))+")) ";

                    if(upper >= 0)
                        lemma += "(<= "+it->first->name+" "+ std::to_string(to_long(upper))+")) ";
                    else
                        lemma += "(<= "+it->first->name+" (- "+ std::to_string(-to_long(upper))+"))) ";
                }
                else if(!ninf(itvs[i])){
                    poly::Integer lower = getRealLower(itvs[i]);
                    if(lower >= 0)
                        lemma += "(>= "+it->first->name+" "+ std::to_string(to_long(lower))+") ";
                    else
                        lemma += "(>= "+it->first->name+" (- "+ std::to_string(-to_long(lower))+")) ";
                }
                else if(!pinf(itvs[i])){
                    poly::Integer upper = getRealUpper(itvs[i]);
                    if(upper >= 0)
                        lemma += "(<= "+it->first->name+" "+ std::to_string(to_long(upper))+") ";
                    else
                        lemma += "(<= "+it->first->name+" (- "+ std::to_string(-to_long(upper))+")) ";
                }
            }
            if(lemma != ""){
                std::cout<<"(assert (or "<< lemma;
                std::cout<<"))\n";
            }
        }
        ++it;
    }
}

void Collector::update_feasible_set(){
    // all update to its sub root
    std::vector<dagc*> vars;
    model->get_variables(vars);
    for(unsigned i=0;i<vars.size();i++){
        dagc* parent = model->getSubRoot(vars[i]);
        if(parent != vars[i]){
            if(variables.find(vars[i]) != variables.end()){
                // the variable has a feasible set
                if(variables.find(parent) == variables.end()){
                    // parent has not a feasible set
                    feasible_set* fs = new feasible_set();
                    variables.insert(std::pair<dagc*, feasible_set*>(parent, fs));
                    fs->addFeasibleSet(*(variables[vars[i]]));
                }
                else{
                    // parent has a feasible set
                    variables[parent]->intersectFeasibleSet(*(variables[vars[i]]));
                    if(variables[parent]->isSetEmpty()){
                        state = State::UNSAT;
                    }
                }
            }
        }
    }
}
