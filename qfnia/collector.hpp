/* collector.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _COLLECTOR_HPP
#define _COLLECTOR_HPP

#include "frontend/parser.hpp"
#include "rewriters/rewriter.hpp"
#include "utils/model.hpp"
#include "utils/feasible_set.hpp"
#include "utils/graph.hpp"
#include "utils/disjoint_set.hpp"
#include <boost/unordered_set.hpp>

namespace ismt
{
    class Collector: public rewriter
    {
    public:

        State state = State::UNKNOWN;

        // graph objects
        Graph<std::string> mustNeqGraph;                // must node neq-graph
        Graph<std::string> MayNeqGraph;                 // may node neq-graph
        std::vector<dagc*> mustNodes;                   // list for must nodes.
        std::vector<dagc*> mayBNodes;                   // list for may bound nodes.
        // used times of variables
        boost::unordered_map<dagc*, unsigned int> used; // collect used times.

        // integer variables and their feasible sets
        boost::unordered_map<dagc*, feasible_set*> variables;   // must bound
        boost::unordered_map<dagc*, feasible_set*> mvariables;  // may bound
        // 100*x + y = 10 -> MaxNumber[x] = 0, MaxNumber[y] = 100
        boost::unordered_map<dagc*, Integer> MaxNumber;         // abs max number met
        boost::unordered_map<dagc*, unsigned> bits; // candidate bits (distinct graph)
        boost::unordered_map<dagc*, unsigned> mul_var_count; // count linear/non-linear mul node of variable

        // disjoint_set of expr
        disjoint_set<dagc*> subSet;                     // if must sat, -n2*n6+n18 = 0 -> n18 = n2*n6. 

        boost::unordered_set<dagc*> unused_vars;        // record unused variables and for future set model

    private:
        model_s* model = nullptr;
        int max_number_bound = 32; // ignore the constraint with x nodes. 
        // collect must, may and neq nodes + rewriting comp.
        bool collect(dagc* root, bool must = true);     // entry of collecting must, may and neq nodes.
        void collectNeq(dagc* root, bool must = false); // only to find neq.
        void collectMax(dagc* root);                    // abs max number met
        void collectMaxC(dagc* root, boost::unordered_map<dagc*, Integer>& dimap); // set the max number of variable in the same constraint
        void collectMaxCMul(dagc* root, boost::unordered_set<dagc*>& vars, Integer& maximum); // set the max number of variable in mul
        
        // analyze each comp constraint
        bool isTrivialComp(dagc* root);
        bool isSimpleComp(dagc* root);
        bool isEasyIte(dagc* root);                         // ite ? 1 0
        bool setEasyIte(dagc* a, dagc* b);                  // b is easy ite
        // 0: false; x: the number of variables. 
        int isOrInterval(dagc* root);                       
        void doOrInterval(dagc* root);                       
        bool setBound(dagc* d, dagc* var, 
                Integer ans, bool must);                    // set bound for var
        bool analyze(dagc* root, bool isTop = true);        // analyze each comp constraint
        void addInterval(dagc* var, bool must, 
            const Interval& interval, bool neq = false);    // add interval to message
        bool doIntervalLit(dagc* root, bool isTop = true);  // deal with interval literal

        void collect();
        bool removeVars();
        bool analyze();
        bool check();   // check must sat node at first.
        bool set_model();
        unsigned int usedTimes(dagc* var);
        int count_node(dagc* root);
        int count_mul(dagc* root);
        int count_add(dagc* root);
        int count_var(dagc* root);
    public:
    
        Collector(Parser* p, model_s* m): rewriter(p), model(m){
            // init
            for(size_t i=0;i<data->vbool_list.size();i++){
                used.insert(std::pair<dagc*, unsigned int>(data->vbool_list[i], 0));
            }
            for(size_t i=0;i<data->vnum_int_list.size();i++){
                used.insert(std::pair<dagc*, unsigned int>(data->vnum_int_list[i], 0));
            }
            for(size_t i=0;i<data->vnum_real_list.size();i++){
                used.insert(std::pair<dagc*, unsigned int>(data->vnum_real_list[i], 0));
            }
        }
        ~Collector(){
            // delete
            auto it = variables.begin();
            while(it!=variables.end()){
                delete it->second;
                it->second = nullptr;
                ++it;
            }
            it = mvariables.begin();
            while(it!=mvariables.end()){
                delete it->second;
                it->second = nullptr;
                ++it;
            }
        }
        // collect useful informations
        //  1. distinct graph
        //  2. must/may sat constraints
        bool rewrite();
        int get_must_distinct_bit(dagc* var);
        int get_may_distinct_bit(dagc* var);
        int get_max_number_bit(dagc* var);
        int get_max_number_count();
        int get_may_constraint_bit(dagc* var);
        int get_var_mul_count(dagc* var);
        int count_mul();
        int count_add();
        int count_node();

        // print smtlib
        void print_feasible_set();
        void update_feasible_set();
    };
    
} // namespace ismt


#endif