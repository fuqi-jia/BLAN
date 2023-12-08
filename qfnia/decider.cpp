/* decider.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "qfnia/decider.hpp"
#include <cmath>
#include <map>
#include <algorithm>

using namespace ismt;

#define decideDebug 0
#define assignDebug 0

// 0 -> lower; 1 -> mid
const int offset_start = 0;

Decider::Decider(){}
Decider::~Decider(){
    auto it = watch_pool.begin();
    while(it!=watch_pool.end()){
        delete *it; *it = nullptr;
        ++it;
    }
}

watchers* Decider::get_watchers(unsigned index){
    return watch_pool[index];
}
watchers* Decider::get_watchers(dagc* var){
    if(watch_map.find(var) != watch_map.end()){
        return get_watchers(watch_map[var]);
    }
    return nullptr; 
}
watch* Decider::get_atom(unsigned index){
    return atom_pool[index];
}
watch* Decider::get_atom(dagc* atom){
    if(atom_map.find(atom) != atom_map.end()){
        return get_atom(atom_map[atom]);
    }
    return nullptr;
}
int Decider::get_atom_i(dagc* atom){
    if(atom_map.find(atom) != atom_map.end()){
        return atom_map[atom];
    }
    return -1;
}
int Decider::get_watchers_i(dagc* var){
    if(watch_map.find(var) != watch_map.end()){
        return watch_map[var];
    }
    return -1;
}
void Decider::collect(dagc* cur, dagc* top){
    if(cur->isvar() && !cur->isAssigned()){
        // cur is a variable and cur is not assigned in preprocessor.
        watchers* res = get_watchers(cur);
        if(res == nullptr){
            watchers* w = new watchers(cur);
            size_t size = watch_pool.size();
            watch_map.insert(std::pair<dagc*, unsigned>(cur, size));
            watch_pool.emplace_back(w);
            res = w;
        }
        int topi = get_atom_i(top);
        assert(topi!=-1);
        ++(atom_pool[topi]->count);
        res->data.emplace_back((unsigned)topi);
    }
    else{
        auto it = cur->children.begin();
        while(it != cur->children.end()){
            collect(*it, top);
            ++it;
        }
    }
}

// 2023-05-19: collector will remove like x >= 3 and if x never appear again, it will make errors.
void Decider::collect_unconstrained_vars(){
    // 2023-05-23: lose an assigned variable but is not subRoot
    std::vector<dagc*> vars;
    info->message->model->getRestVars(vars);
    for(size_t i=0;i<vars.size();i++){
        if(!get_watchers(vars[i])){
            watchers* w = new watchers(vars[i]);
            size_t size = watch_pool.size();
            watch_map.insert(std::pair<dagc*, unsigned>(vars[i], size));
            watch_pool.emplace_back(w);
        }
    }
    // auto it = info->collector->variables.begin();
    // while(it!=info->collector->variables.end()){
    //     // 2023-05-22: if a variable is assigned, we cannot set it as a variable.
    //     // if(it->first->isAssigned()){ ++it; continue;}
    //     if(info->message->model->has(it->first)) {++it; continue;}
    //     if(!get_watchers(it->first)){
    //         watchers* w = new watchers(it->first);
    //         size_t size = watch_pool.size();
    //         watch_map.insert(std::pair<dagc*, unsigned>(it->first, size));
    //         watch_pool.emplace_back(w);
    //     }
    //     ++it;
    // }
}

void Decider::init(Info* i){
    info = i;
    // collect watches;
    auto it = info->message->constraints.begin();
    while(it != info->message->constraints.end()){
        size_t size = atom_pool.size();
        atom_map.insert(std::pair<dagc*, unsigned>(*it, size));
        atom_pool.emplace_back(new watch(*it));
 
        // collect all vars
        collect(*it, *it);
        ++it;
    }

    // 2023-05-18: collect all variables are removed by collector
    collect_unconstrained_vars();
    
    for(size_t i=0;i<atom_pool.size();i++){
        atom_pool[i]->set_origin();
    }
    resort();

    // 2023-05-29
    if(info->options->MaxBW != (int)max_width){
        max_width = info->options->MaxBW > 0?(unsigned)info->options->MaxBW:max_width;
    }

    // set default bit width
    mul_bound.emplace_back(256); mul_bound.emplace_back(512); mul_bound.emplace_back(1024);
    start_bit.emplace_back(6); start_bit.emplace_back(5); start_bit.emplace_back(4); start_bit.emplace_back(2);
    re_factor_bit.emplace_back(4); re_factor_bit.emplace_back(3); re_factor_bit.emplace_back(2); re_factor_bit.emplace_back(1);
    rephase_bit.emplace_back(4); rephase_bit.emplace_back(4); rephase_bit.emplace_back(3); rephase_bit.emplace_back(2);
    int mul_cnt = info->collector->count_mul();
    for(unsigned i=0;i<mul_bound.size();i++){
        if(mul_cnt < mul_bound[i]){
            def_width = start_bit[i];
            re_factor = re_factor_bit[i];
            rephase_factor = rephase_bit[i];
            break;
        }
    }
    if(mul_cnt >= mul_bound.back()){
        def_width = start_bit.back();
        re_factor = re_factor_bit.back();
        rephase_factor = rephase_bit.back();
    }

    // 2023-05-27
    if(info->options->MA){
        def_width = std::max(2, info->options->Beta - (int)std::ceil((double)mul_cnt / (double)info->options->Alpha));
    }

    // 2023-05-27
    if(info->options->VO){
        // // vote the bits
        vote();
    }


    // new solver
    icp = new icp_solver(info->prep->polyer);
}

Interval Decider::mk_interval(unsigned i){
    return poly::Interval(-(long)ceil(pow(2, i)) + 1, false, (long)ceil(pow(2, i)) - 1, false);
}
Interval Decider::mk_interval(const poly::Integer& value, unsigned i, bool is_lower){
    if(is_lower){
        if(offset_start == 0){
            return poly::Interval(value, false, value + poly::Integer((long)ceil(pow(2, i)) - 1), false);
        }
        else if(offset_start == 1){
            return poly::Interval(value, false, value + poly::Integer((long)ceil(pow(2, i)) - 2), false);
        }
        else assert(false);
    }
    else{
        if(offset_start == 0){
            return poly::Interval(value - poly::Integer((long)ceil(pow(2, i)) + 1), false, value, false);
        }
        else if(offset_start == 1){
            return poly::Interval(value - poly::Integer((long)ceil(pow(2, i)) + 2), false, value, false);
        }
        else assert(false);
    }
}

void Decider::assign_bit(int bit){
    assert(bit > 0);
    Interval itv = mk_interval(bit - 1);
    default_widths.emplace_back(bit);
    assignment.emplace_back(itv);
}

void Decider::assign_full(dagc* xvar){
    int bit = get_bit(xvar);

    // default interval
    assign_bit(bit); // defualt
}

int Decider::get_candidate_bit(dagc* xvar, bool zero){
    if(voted == 2){ return bit_vote; }
    if(candidate_bit.find(xvar) != candidate_bit.end()) return candidate_bit[xvar];
    int bit = -1;
    int bit0=0; int bit1=0;
    // 2023-05-27
    if(info->options->DG){
        // distinct graph
        bit0 = info->collector->get_must_distinct_bit(xvar);

        // may distinct graph
        bit1 = info->collector->get_may_distinct_bit(xvar);
    }

    // // or constraint
    // int bit2 = info->collector->get_may_constraint_bit(xvar);

    // // // max number of variable
    // int bit3 = 0;
    int bit3 = 0;
    // 2023-05-27
    if(info->options->CM){
        bit3 = info->collector->get_max_number_bit(xvar);
        if(zero) bit3 += 1;
    }
    // it can not be a major judgement
    // 2023-05-27
    if(bit3 > info->options->K) bit3 = info->options->K; 

    bit = std::max({bit0, bit1, bit3}); 
    if(bit > 0){
        candidate_bit.insert(std::pair<dagc*, int>(xvar, bit));
        return bit;
    }
    
    return -1;
}

void Decider::vote(){
    boost::unordered_map<int, int> count;
    int max_cnt = 0;
    int max_bit = 0;
    for(unsigned i=0;i<current.size();i++){
        dagc* x = current[i]->var->var;
        int bit = get_bit(x);
        if(count.find(bit) == count.end()){
            count.insert(std::pair<int, int>(bit, 1));
        }
        else{
            count[bit] += 1;
            if(count[bit] > max_cnt){
                // leave the max occurance
                max_bit = bit;
                max_cnt = count[bit];
            }
            else if(count[bit] == max_cnt){
                // leave the max bit
                if(max_bit < bit){
                    max_bit = bit;
                }
            }
        }
    }

    // 2023-05-27
    if(max_cnt > (int)(info->options->Gamma * current.size())){
        voted = 2;
        bit_vote = max_bit;
        def_width = std::max(def_width, (unsigned)bit_vote); // 06/04
    }
    else{
        voted = 1;
    }
}

int Decider::get_bit(dagc* xvar){
    // // default bit, candidate bit, phase bit, assigned bit
    // int abit = assign_index.find(xvar)==assign_index.end()?re_factor:default_widths[assign_index[xvar]] + re_factor;
    int cbit = get_candidate_bit(xvar);
    int dbit = (int)def_width;  
    // int pbit = phase_bit.find(xvar)==phase_bit.end()?0:std::min(phase_bit[xvar] + rephase_factor, (int)max_width);
    // // std::cout<<abit<<" "<<cbit<<" "<<dbit<<" "<<pbit<<std::endl;
    int ret_bit = std::max({cbit, dbit});
    if(ret_bit > (int)max_width){
        b_conflict = true;
        ret_bit = max_width;
    }
    return ret_bit;
    // if(!xvar) return 0;
    // return def_width;
}
bool Decider::re_assign(dagc* xvar){
    int index = assign_index.find(xvar)==assign_index.end()?-1:assign_index[xvar];
    assert(index != -1);
    Interval i = assignment[index];
    int bit = get_bit(xvar);
    if(b_conflict) return false;
    
    feasible_set* fs = nullptr;
    if(info->collector->variables.find(xvar) != info->collector->variables.end()){
        fs = info->collector->variables[xvar];
    }

    if(fs != nullptr){
        // Interval i = fs->choose();
        Interval i = fs->get_cover_interval();
        if(isFull(i)){
            Interval itv = mk_interval(bit);
            default_widths[index] = bit;
            assignment[index] = itv;
        }
        else if(ninf(i) || pinf(i)){
            Interval itv;
            if(ninf(i)) itv = mk_interval(getUpper(i), bit, false);
            else itv = mk_interval(getLower(i), bit, true);
            default_widths[index] = bit;
            assignment[index] = itv;
        }
        else if(isPoint(i)){}
        else{}
    }
    else{
        Interval itv = mk_interval(bit);
        default_widths[index] = bit;
        assignment[index] = itv;
    }

    #if assignDebug
        std::cout<<"re-assign: "<<xvar->name<<" -> "<<assignment[index]<<std::endl;
    #endif
    info->assignment[xvar] = assignment[index];
    icp->assign(xvar, assignment[index]);
    return true;
}
bool Decider::assign(dagc* xvar){
    if(xvar->isvbool()) return true;

    // from info->collector
    feasible_set* fs = nullptr;
    if(info->collector->variables.find(xvar) != info->collector->variables.end()){
        fs = info->collector->variables[xvar];
    }

    if(fs != nullptr){
        // Interval i = fs->choose();

        Interval i = fs->get_cover_interval();
        // Interval i;
        // if(fs->size() > 1){
        //     i = fs->choose();
        // }
        // else{
        //     // size <= 1
        //     i = fs->get_cover_interval();
        // }
        if(isFull(i)){
            assign_full(xvar);
        }
        else if(ninf(i) || pinf(i)){
            Interval itv;
            int bit = get_bit(xvar);
            if(ninf(i)) itv = mk_interval(getUpper(i), bit, false);
            else itv = mk_interval(getLower(i), bit, true);
            default_widths.emplace_back(bit);
            assignment.emplace_back(itv);
        }
        else if(isPoint(i)){
            // [c, c] -> [c, c]
            default_widths.emplace_back(0);
            assignment.emplace_back(i);
        }
        else{
            // bounded
            unsigned l = poly::log_size(i);
            default_widths.emplace_back(l);
            assignment.emplace_back(i);
        }
    }
    else{
        assign_full(xvar);
    }
    info->assignment.insert(std::pair<dagc*, Interval>(xvar, assignment.back()));
    
    #if assignDebug
        std::cout<<"assign: "<<xvar->name<<" -> "<<assignment.back()<<std::endl;
    #endif
    assign_index.insert(std::pair<dagc*, unsigned>(xvar, assignment.size()-1));
    icp->assign(xvar, assignment.back());

    return true;
}

bool Decider::conflict() const {
    return b_conflict;
}

void Decider::count_mul(dagc* cur, int& val){
    if(cur->ismul()){
        val += 1;
        return;
    }
    else{
        for(unsigned i=0;i<cur->children.size();i++){
            count_mul(cur->children[i], val);
        }
    }
}
bool Decider::too_big(dagc* cur){
    // expand form polynomial
    // check the number of multiplier
    int cnt = 0;
    count_mul(cur, cnt);
    if(cnt > max_mul_esc_icp) return true;
    else return false;
}
bool Decider::propagate(std::vector<dagc*>& cs){
    if(cs.size() == 0) return true;
    std::vector<dagc*> new_cs;
    // incremental check
    for(unsigned i=0;i<cs.size();i++){
        if(!too_big(cs[i])){
            new_cs.emplace_back(cs[i]);
            #if decideDebug
                info->message->data->printAST(cs[i]);
                std::cout<<std::endl;
            #endif
        }
    }

    if(icp->check(new_cs) == State::SAT){
        #if decideDebug
            std::cout<<"* -> true."<<std::endl;
        #endif
        // because all assigned, we can use icp to check
        return true;
    }
    #if decideDebug
        std::cout<<"* -> false."<<std::endl;
    #endif
    return false;
}

void Decider::increment(const unsigned& idx){
    default_widths[idx] *= 2;
}
void Decider::increment(){
    resort();
    for(size_t i=0;i<atom_pool.size();i++){
        atom_pool[i]->reset();
    }
    icp->reset();
    info->reset();
    def_width = std::min(max_width, def_width*2);
}

void Decider::collect_vars(dagc* cur, boost::unordered_set<dagc*>& vars){
    if(cur->isvnum()){ // only number variables
        vars.insert(cur);
    }
    else if(!too_big(cur)){
        auto it = cur->children.begin();
        while(it != cur->children.end()){
            collect_vars(*it, vars);
            ++it;
        }
    }
}
bool Decider::backtrack(std::vector<dagc*>& cs){
    // find var set and increment the interval
    boost::unordered_set<dagc*> vars;
    for(unsigned i=0;i<cs.size();i++){
        collect_vars(cs[i], vars);
    }
    bool ans = true;
    // in an order of time of assigning
    for(unsigned i=0;i<info->variables.size();i++){
        dagc* x = info->variables[i];
        if(vars.find(x) != vars.end()){
            ans |= re_assign(x);
            // if(propagate(cs)) break; // new 05/30 
        }
    }
    if(!ans) return false;
    return true;

    // auto it = vars.begin();
    // while(it != vars.end()){
    //     ans |= re_assign(*it, assign_index[*it]);
    //     // if(propagate(cs)) break; // new 05/30 
    //     ++it;
    // }
    // if(!ans) return false;
    // return true;
}

bool Decider::decide(){
    if(def_width > max_width){ b_conflict = true; return false; }
    if(current.size() == 0) { 
        is_done = true;
        return false; // all decided
    }

    watchers* w = current.back();
    // select the variable
    trail.emplace_back(w);
    levels.emplace_back(trail.size());
    info->new_vars.emplace_back(w->var->var);
    info->variables.emplace_back(w->var->var);
    current.pop_back();

    // collect constraints
    std::vector<dagc*> cs;
    for(unsigned j=0;j<w->size();j++){
        watch* cur = get_atom(w->data[j]);
        cur->assign();
        if(cur->is_ok()){
            cs.emplace_back(cur->atom);
        }
    }
    
    // assign right now
    dagc* xvar = w->var->var;
    if(!assign(xvar)){
        info->state = State::UNSAT;
        return false;
    }

    if(cs.size() == 0) return true;
    // append to info
    for(size_t i=0;i<cs.size();i++){
        info->new_constraints.emplace_back(cs[i]);
    }

    if(xvar->isvbool()){    
        #if assignDebug
            std::cout<<"assign bool: "<<xvar->name<<std::endl;
        #endif
        return true;
    }

    // // icp checking...
    // while(true){
    //     if(propagate(cs)) break;
    //     else if(!backtrack(cs)) return false;
    // }
    // #if decideDebug
    //     std::cout<<"decide done."<<std::endl;
    // #endif

    return true;
}
bool Decider::done(){
    return is_done;
}

void Decider::resort(){
    is_done = false;
    trail.clear();
    levels.clear();
    auto it = assign_index.begin();
    while(it!=assign_index.end()){
        dagc* x = it->first;
        int bit = default_widths[it->second];

        if(phase_bit.find(x) != phase_bit.end()){
            phase_bit.insert(std::pair<dagc*, int>(x, bit));
        }
        else{
            phase_bit[x] = bit;
        }
        ++it;
    }
    assign_index.clear();
    default_widths.clear();
    std::sort(watch_pool.begin(), watch_pool.end());
    for(unsigned i=0;i<watch_pool.size();i++){
        current.emplace_back(watch_pool[i]);
        dagc* var = watch_pool[i]->var->var;
        var->unassign();
        if(watch_map.find(var) != watch_map.end()){
            watch_map[var] = i;
        }
        else{
            watch_map.insert(std::pair<dagc*, unsigned>(var, i));
        }
    }
}
