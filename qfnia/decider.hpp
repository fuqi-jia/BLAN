/* decider.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _DECIDER_H
#define _DECIDER_H

#include "qfnia/info.hpp"
#include "solvers/icp/icp_solver.hpp"

#include <stack>
#include <boost/unordered_set.hpp>

namespace ismt
{
    // search box decider
    typedef unsigned dvar;

    struct priority{
        dagc* var;
        unsigned value;
        priority(dagc* v):var(v){}
        ~priority(){}
        bool operator<(const priority& p){
            return value < p.value;
        }
    };

    struct watch{
        dagc* atom  = nullptr;
        int count   = 0;
        int origin  = 0;
        watch(dagc* a):atom(a){}
        ~watch(){}
        void assign(){ --count; }
        void unassign(){ ++count; }
        bool is_ok(){ return count == 0; }
        void set_origin(){ origin = count; }
        void reset(){ count = origin; }
    };

    struct watchers{
        priority* var = nullptr;
        std::vector<unsigned> data;
        watchers(dagc* v){
            var = new priority(v);
        }
        ~watchers(){
            delete var; var = nullptr;
        }
        bool operator<(const watchers& w){
            return var->value > w.var->value;
        }
        size_t size() const { return data.size(); }
    };

    class Decider
    {
    private:
        bool b_conflict = false;
        unsigned max_width = 32;
        unsigned nvars = 0;
        unsigned def_width = 2; // 2 2022-05-18; 3 2022-06-01
        unsigned re_factor = 1;
        int number_clip = 4;
        int voted = 0; // 0 -> not do; 1 -> done but not used; 2 -> done and used  
        int bit_vote = 0;
        float vote_bound = 0.5;
        boost::unordered_map<dagc*, int> candidate_bit;
        boost::unordered_map<dagc*, int> phase_bit;
        int rephase_factor = 4; // rephase factor
        std::vector<int> mul_bound;
        std::vector<int> start_bit;
        std::vector<int> re_factor_bit;
        std::vector<int> rephase_bit;
        bool is_done = false;
        Info* info = nullptr;
        icp_solver* icp = nullptr;
        poly::IntervalAssignment* data = nullptr;
        std::vector<watch*> atom_pool;
        // atom -> index
        boost::unordered_map<dagc*, unsigned> atom_map; 
        std::vector<Interval> assignment;
        boost::unordered_map<dagc*, unsigned> assign_index;
        std::vector<watchers*> watch_pool; 
        // var -> watchers index
        boost::unordered_map<dagc*, unsigned> watch_map;
        std::vector<watchers*> current;

        std::vector<watchers*> trail;
        std::vector<unsigned> levels;

        void collect            (dagc* cur, dagc* top);
        void collect_unconstrained_vars  ();
        void watch_atom         (dagc* cur);
        bool assign             (dagc* var);
        bool too_big            (dagc* cur);
        
        int  max_mul_esc_icp    = 128;
        void count_mul          (dagc* cur, int& val);
        // void find_max_var_level (std::vector<dagc*>& cs, int& value);

        int get_atom_i          (dagc* atom);
        int get_watchers_i      (dagc* var);
        watch*    get_atom      (unsigned index);
        watchers* get_watchers  (unsigned index);
        watch*    get_atom      (dagc* atom);
        watchers* get_watchers  (dagc* var);
        Interval  mk_interval   (unsigned i);
        Interval  mk_interval   (const poly::Integer& value, unsigned i, bool is_lower);
        void      assign_full   (dagc* xvar);
        void      assign_bit    (int bit);

        bool      backtrack     (std::vector<dagc*>& cs);
        bool      re_assign     (dagc* xvar);
        void      collect_vars  (dagc* cur, boost::unordered_set<dagc*>& vars);
        int       get_candidate_bit(dagc* xvar, bool zero = true);

        // default bit width
        std::vector<unsigned> default_widths;

    public:
        Decider                 ();
        ~Decider                ();

        bool propagate          (std::vector<dagc*>& cs);
        // bool backtrack          (std::vector<dagc*>& cs);
        void init               (Info* i);

        bool decide             ();
        bool done               ();

        void resort             ();
        void increment          ();
        void vote               ();
        void increment          (const unsigned& idx);
        bool conflict           () const;
        int get_bit             (dagc* xvar);
    };
    
} // namespace ismt


#endif

// decide done.
// Number of variables: 82982
// Number of clauses: 246074
// assert done...
// literals: 234109
// clauses: 930035