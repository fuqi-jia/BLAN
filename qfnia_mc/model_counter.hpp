/* model_counter.hpp
*
*  Copyright (C) 2025 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _MODEL_COUNTER_HPP
#define _MODEL_COUNTER_HPP

// frontend
#include "frontend/parser.hpp"
// midend
#include "midend/preprocessor.hpp"
#include "midend/message.hpp"
// backend
#include "qfnia/info.hpp"
#include "qfnia/collector.hpp"
#include "qfnia/decider.hpp"
#include "qfnia/resolver.hpp"
#include "qfnia/searcher.hpp"
#include <string>

namespace ismt
{   
namespace ismt
{
    // static poly::Context context();
    const int maxVars = 256;
    class model_counter{
    private:
        DAG* data;
        bool used = false;
        std::string benchmark;
        State state = State::UNKNOWN;
        // midend
        preprocessor* prep = nullptr;
        // qfnia part
        Collector* collector = nullptr;
        Decider* decider = nullptr;
        Resolver* resolver = nullptr;
        Searcher* searcher = nullptr;
        
        // 私有方法声明
        long long convert_to_dnnf_and_count();
        int perform_cad_decomposition();
        int perform_bit_blasting(int cad_cells);
        int call_ganak_solver();
        bool generate_cnf_file(const std::string& filename);
        int parse_ganak_output(const std::string& filename);
        int call_approximate_counter();
        dagc* get_root_constraint();
        int estimate_model_count();
        
    public:
        model_counter();
        ~model_counter();
        // main function
        void init(Info* info);
        int count_model();
    };
} // namespace ismt


#endif