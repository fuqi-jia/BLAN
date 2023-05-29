/* model.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _MODEL_REWRITER_H
#define _MODEL_REWRITER_H

#include "frontend/dag.hpp"
#include "utils/types.hpp"
#include "utils/disjoint_set.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

namespace ismt
{
    // 1. x = 3
    // 2. x = y
    // 3. x = y + 18, x = 18*y, not supported.

    class model_s
    {
    private:
        disjoint_set<dagc*> subSet;
        std::vector<dagc*> variables;
        std::vector<unsigned> subMap;
        bool changed = false;
        size_t unassigned = 0;
    public:
        bool consistent = true;
        boost::unordered_map<dagc*, unsigned> modelMap; 
        boost::unordered_map<std::string, unsigned> nameMap;
    public:
        model_s(/* args */){}
        ~model_s(){}
        
        // set value for variable.
        bool set(dagc*& var, const Integer& value){
            changed = true;
            size_t idx = modelMap[var];
            if(var->isAssigned()){
                if(var->v != value){
                    consistent = false;
                    return false;
                }
                else return true;
            }
            variables[idx]->assign(value);
            --unassigned;
            dagc* target = subSet.find(var);
            if(target != var) return set(target, value);
            else return true;
        }
        bool set(const std::string& var_name, const std::string& val){
            Integer value = 0;
            if(val == "true"){
                value = 1;
            }
            else if(val == "false"){
                value = 0;
            }
            else{
                // integer
                value = Integer(val.c_str()); // string?
            }
            changed = true;
            size_t idx = nameMap[var_name];
            dagc* var = variables[idx];
            if(var->isAssigned()){
                if(var->v != value){
                    consistent = false;
                    return false;
                }
                else return true;
            }
            variables[idx]->assign(value);
            --unassigned;
            dagc* target = subSet.find(var);
            if(target != var) return set(target, value);
            else return true;
        }
        // substitute map, store the most currently.
        bool substitute(dagc*& a, dagc*& b){
            // must the same type
            assert(a->t==b->t);

            dagc* ra = getSubRoot(a);
            dagc* rb = getSubRoot(b);
            if(a->isAssigned() && b->isAssigned()){
                if(a->v != b->v) return false;
                // a->v == b->v
                if(ra->isAssigned() && a->v != ra->v) return false;
                if(rb->isAssigned() && a->v != rb->v) return false;
            }
            else if(a->isAssigned()){
                b->assign(a->v);
                if(ra->isAssigned() && a->v != ra->v) return false;
                if(rb->isAssigned() && a->v != rb->v) return false;
            }
            else if(b->isAssigned()){
                a->assign(b->v);
                if(ra->isAssigned() && b->v != ra->v) return false;
                if(rb->isAssigned() && b->v != rb->v) return false;
            }
            else{
                if(ra->isAssigned() && rb->isAssigned()){
                    if(ra->v != rb->v) return false;
                    // ra->v == rb->v
                    a->assign(ra->v);
                    b->assign(ra->v);
                }
                else if(ra->isAssigned()){
                    rb->assign(ra->v);
                    a->assign(ra->v);
                    b->assign(ra->v);
                }
                else if(rb->isAssigned()){
                    ra->assign(rb->v);
                    a->assign(rb->v);
                    b->assign(rb->v);
                }
            }

            changed = true;
            // so if (bool a) and (int a) -> it will cause an error.
            subSet.add(a);
            subSet.add(b);
            subSet.union_set(a, b);
            // is subsitituted
            subMap[modelMap[a]] = 1;
            subMap[modelMap[b]] = 1;

            return true;
        }

        bool has(dagc*& var){
            if(var->isAssigned()){
                // 2023-05-22: update subroot
                if(getSubRoot(var) != var && !getSubRoot(var)->isAssigned()){
                    dagc* p = getSubRoot(var);
                    p->assign(var->v);
                }
                return true;
            }
            else{
                dagc* p = getSubRoot(var);
                if(p->isAssigned()){
                    var->assign(p->v);
                    return true;
                }
                return false;
            }
        }
        
        bool done() const { return unassigned == 0;}

        // 2023-05-22: assigned variables
        void getAssignedVars(std::vector<dagc*>& vars){
            update();
            vars.clear();
            for(size_t i=0;i<variables.size();i++){
                if(variables[i]->isAssigned()){
                    vars.emplace_back(variables[i]);
                }
            }
        }
        // 2023-05-22: unassigned variables (include some substituted variables)
        void getUnassignedVars(std::vector<dagc*>& vars){
            update();
            vars.clear();
            for(size_t i=0;i<variables.size();i++){
                if(!variables[i]->isAssigned()){
                    vars.emplace_back(variables[i]);
                }
            }
        }
        // 2023-05-22: all rest variables should be declared (not include some substituted variables)
        void getRestVars(std::vector<dagc*>& vars){
            update();
            vars.clear();
            for(size_t i=0;i<variables.size();i++){
                // !has: means that the variable is not assigned
                // getSubRoot(variables[i]) == variables[i]: means that the variable is the subroot
                // sometimes subroot will be eliminated so that should be clearly declared.
                if(!has(variables[i]) && getSubRoot(variables[i]) == variables[i]){
                    vars.emplace_back(variables[i]);
                }
            }
        }
        // 2023-05-22: update the assignment to subroot
        void update(){
            for(size_t i=0;i<variables.size();i++){
                if( variables[i]->isAssigned() && 
                    getSubRoot(variables[i]) != variables[i] && 
                    !getSubRoot(variables[i])->isAssigned()){
                    dagc* p = getSubRoot(variables[i]);
                    p->assign(variables[i]->v);
                }
            }
        }

        dagc* getSubRoot(dagc*& var){
            return subSet.find(var);
        }
        bool isSub(dagc* var){
            return subMap[modelMap[var]] == 1;
        }
        Integer getValue(dagc*& var){
            if(var->isAssigned()) return var->v;
            else{
                dagc* p = getSubRoot(var);
                if(p->isAssigned()){
                    var->assign(p->v);
                }
                else assert(false);
                return p->v;
            }
        }

        size_t size() const { return variables.size(); }
        dagc*& operator[](size_t i){ return variables[i]; }

        bool isChanged(){
            return changed;
        }
        void clear(){
            variables.clear();
            modelMap.clear();
            subSet.clear();
            subMap.clear();
            nameMap.clear();
        }
        void setVariables(const std::vector<dagc*>& vars){
            for(size_t i=0;i<vars.size();i++){
                if(modelMap.find(vars[i]) != modelMap.end()) continue;
                modelMap.insert(std::pair<dagc*, unsigned>(vars[i], variables.size()));
                nameMap.insert(std::pair<std::string, unsigned>(vars[i]->name, variables.size()));
                variables.emplace_back(vars[i]);
            }
            unassigned = variables.size();
            subMap.resize(unassigned, 0);
        }
        void print(){
            std::cout<<"(model"<<std::endl;
            for(size_t i=0;i<variables.size();i++){
                dagc* var = subSet.find(variables[i]);
                if(var!=variables[i]){
                    if(variables[i]->isvbool()){
                        std::cout<<" (define-fun "<<variables[i]->name<<" () Bool "<< (var->v==1?"true":"false") <<")\n";
                    }
                    else if(variables[i]->isvnum()){
                        if(var->v >= 0){
                            std::cout<<" (define-fun "<<variables[i]->name<<" () Int "<< var->v <<")\n";
                        }
                        else{
                            std::cout<<" (define-fun "<<variables[i]->name<<" () Int (- "<< abs(var->v) <<"))\n";
                        }
                    }
                }
                else{
                    if(variables[i]->isvbool()){
                        std::cout<<" (define-fun "<<variables[i]->name<<" () Bool "<< (variables[i]->v==1?"true":"false") <<")\n";
                    }
                    else if(variables[i]->isvnum()){
                        if(variables[i]->v >= 0){
                            std::cout<<" (define-fun "<<variables[i]->name<<" () Int "<< variables[i]->v <<")\n";
                        }
                        else{
                            std::cout<<" (define-fun "<<variables[i]->name<<" () Int (- "<< abs(variables[i]->v) <<"))\n"; 
                        }
                    }
                }
            }
            std::cout<<")"<<std::endl;
        }
        void print_partial(){
            for(size_t i=0;i<variables.size();i++){
                if(!has(variables[i])) continue;
                dagc* var = subSet.find(variables[i]);
                if(var!=variables[i]){
                    if(variables[i]->isvbool()){
                        std::cout<<"(define-fun "<<variables[i]->name<<" () Bool "<< (var->v==1?"true":"false") <<")\n";
                    }
                    else if(variables[i]->isvnum()){
                        if(var->v >= 0){
                            std::cout<<"(define-fun "<<variables[i]->name<<" () Int "<< var->v <<")\n";
                        }
                        else{
                            std::cout<<"(define-fun  "<<variables[i]->name<<" () Int (- "<< abs(var->v) <<"))\n"; 
                        }
                    }
                }
                else{
                    if(variables[i]->isvbool()){
                        std::cout<<"(define-fun "<<variables[i]->name<<" () Bool "<< (variables[i]->v==1?"true":"false") <<")\n";
                    }
                    else if(variables[i]->isvnum()){
                        if(variables[i]->v >= 0){
                            std::cout<<"(define-fun "<<variables[i]->name<<" () Int "<< variables[i]->v <<")\n";
                        }
                        else{
                            std::cout<<"(define-fun "<<variables[i]->name<<" () Int (- "<< abs(variables[i]->v) <<"))\n"; 
                        }
                    }
                }
            }
        }
        void print_unassigned(){
            boost::unordered_set<dagc*> print_map;
            for(size_t i=0;i<variables.size();i++){
                if(!has(variables[i])){
                    // if printed
                    if(print_map.find(variables[i]) != print_map.end()) continue;
                    // else
                    print_map.insert(variables[i]);
                    dagc* var = subSet.find(variables[i]);
                    if(var!=variables[i]){
                        if(variables[i]->isvbool()){
                            std::cout<<"(declare-fun "<<variables[i]->name<<" () Bool)\n";
                            if(print_map.find(var) == print_map.end()){
                                // pre declare
                                std::cout<<"(declare-fun "<<var->name<<" () Bool)\n";
                                print_map.insert(var);
                            }
                            std::cout<<"(assert (= "<<variables[i]->name<<" "<<var->name<<"))\n";
                        }
                        else if(variables[i]->isvnum()){
                            std::cout<<"(declare-fun "<<variables[i]->name<<" () Int)\n";
                            if(print_map.find(var) == print_map.end()){
                                // pre declare
                                std::cout<<"(declare-fun "<<var->name<<" () Int)\n";
                                print_map.insert(var);
                            }
                            std::cout<<"(assert (= "<<variables[i]->name<<" "<<var->name<<"))\n";
                        }
                    }
                    else{
                        // self
                        if(variables[i]->isvbool()){
                            std::cout<<"(declare-fun "<<variables[i]->name<<" () Bool)\n";
                        }
                        else if(variables[i]->isvnum()){
                            std::cout<<"(declare-fun "<<variables[i]->name<<" () Int)\n";
                        }
                    }

                }
            }
        }


        // note that parent node must be static
        
        bool processed = false;
        boost::unordered_map<dagc*, unsigned> sub_set;
        std::vector<std::set<dagc*>> sub_pool;
        void process_sub(){
            for(unsigned i=0;i<variables.size();i++){
                dagc* parent = getSubRoot(variables[i]);
                if(parent != variables[i]){
                    // add set
                    if(sub_set.find(parent) != sub_set.end()){
                        sub_pool[sub_set[parent]].insert(variables[i]);
                    }
                    else{
                        sub_set.insert(std::pair<dagc*, unsigned>(parent, sub_pool.size()));
                        sub_pool.emplace_back();
                        sub_pool[sub_set[parent]].insert(parent);
                        sub_pool[sub_set[parent]].insert(variables[i]);
                    }
                }
            }
            processed = true;
        }
        void get_sub_set(dagc* root, std::vector<dagc*>& res){
            // get equal set
            if(!processed){
                process_sub();
            }
            unsigned idx = sub_set[getSubRoot(root)];
            auto it = sub_pool[idx].begin();
            while(it != sub_pool[idx].end()){
                res.emplace_back(*it);
                ++it;
            }
        }

        void get_variables(std::vector<dagc*>& res){
            res.clear();
            for(unsigned i=0;i<variables.size();i++){
                res.emplace_back(variables[i]);
            }
        }
    };
    
} // namespace ismt


#endif
