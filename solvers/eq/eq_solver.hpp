/* eq_solver.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _SUB_REWRITER_H
#define _SUB_REWRITER_H

#include "frontend/dag.hpp"
#include "rewriters/rewriter.hpp"
#include "rewriters/eq_rewriter.hpp"
#include "rewriters/poly_rewriter.hpp"
#include "solvers/poly/poly_solver.hpp"
#include "solvers/icp/icp_solver.hpp"
#include "solvers/sat/sat_solver.hpp"
#include "utils/model.hpp"

// future use an abstract collector to isolate qfnia collector
#include "qfnia/collector.hpp"

namespace ismt
{
    class eq_solver: public rewriter
    {
    private:
        model_s* model;
        State state = State::UNKNOWN;
        // checking 
        boost::unordered_map<dagc*, std::vector<dagc*>*> subMap;

        // rewriters
        eq_rewriter* eqer = nullptr;
        poly_rewriter* polyer = nullptr;

        // solvers
        poly_solver* polyser = nullptr;
        icp_solver* icpser = nullptr;
        sat_solver* satser = nullptr;

        // theory collector
        Collector* thector = nullptr;


        // x + y + z / x*y*z 
        void set(dagc* a, dagc* b);
        void rewrite(dagc* root);
        void collect(dagc* root);
        void collect();
        bool checkEqAtoms(std::vector<dagc*>& closure);
        bool checkEqPolys(std::vector<dagc*>& closure);
        bool checkEqs(std::vector<dagc*>& closure);
        bool check(); // check the closure under equal operator
    public:
        eq_solver(Parser* p, model_s* m): rewriter(p), model(m) {
            data = &(p->dag);
            polyser = new poly_solver();
            // icpser = new icp_solver();
            satser = new sat_solver();
        }
        ~eq_solver(){
            auto it = subMap.begin();
            while(it!=subMap.end()){
                delete it->second; it->second = nullptr;
                ++it;
            }
            // delete icpser; icpser = nullptr;
            delete polyser; polyser = nullptr;
        }

        void setRewriters(eq_rewriter* eq, poly_rewriter* po);
        void setCollector(Collector* c);
        
        // 1. rewriting: 
        //  (= x (+ y 1)) (= z (+ x 1)) -> (= x (+ y 1)) (= z (+ y 2))
        //  (= x y) -> subsititute all x with y.
        // 2. solving:
        //  (= x (+ y 1)) (= y (+ x 1)) -> (= y (+ y 1)) -> false
        //  (!= y 0) (= x (* y y)) (= x (- (* y y))) -> (!= y 0) (= (* y y) (- (* y y))) -> false
        void rewrite(); // 1
        bool solve(); // 1 + 2
    };

} // namespace ismt


#endif
