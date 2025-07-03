#ifndef _D_DNNF_HPP
#define _D_DNNF_HPP

#include "model_counter.hpp"
#include "frontend/dag.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

namespace ismt
{
    // d-DNNF节点类型
    enum class DNNFNodeType {
        AND,
        OR,
        LITERAL,
        TRUE_NODE,
        FALSE_NODE
    };
    
    // d-DNNF节点
    struct DNNFNode {
        DNNFNodeType type;
        std::vector<std::shared_ptr<DNNFNode>> children;
        int literal_id;  // 对于LITERAL节点
        bool is_negated; // 对于LITERAL节点
        
        DNNFNode(DNNFNodeType t) : type(t), literal_id(0), is_negated(false) {}
        DNNFNode(DNNFNodeType t, int lit, bool neg = false) 
            : type(t), literal_id(lit), is_negated(neg) {}
    };
    
    class d_DNNF
    {
    private:
        model_counter* mc = nullptr;
        std::shared_ptr<DNNFNode> root;
        std::unordered_map<dagc*, int> variable_map;
        int next_var_id;
        
        // 转换DAG到d-DNNF的辅助方法
        std::shared_ptr<DNNFNode> convert_dag_to_dnnf(dagc* dag);
        std::shared_ptr<DNNFNode> handle_and_node(dagc* dag);
        std::shared_ptr<DNNFNode> handle_or_node(dagc* dag);
        std::shared_ptr<DNNFNode> handle_not_node(dagc* dag);
        std::shared_ptr<DNNFNode> handle_literal(dagc* dag);
        std::shared_ptr<DNNFNode> handle_comparison(dagc* dag);
        
        // 获取或创建变量ID
        int get_or_create_var_id(dagc* var);
        
        // 优化d-DNNF
        std::shared_ptr<DNNFNode> optimize_dnnf(std::shared_ptr<DNNFNode> node);
        
        // 检查d-DNNF性质
        bool is_decomposable(std::shared_ptr<DNNFNode> node);
        bool is_deterministic(std::shared_ptr<DNNFNode> node);
        
        // Model counting on d-DNNF
        long long count_models_recursive(std::shared_ptr<DNNFNode> node, 
                                        std::unordered_map<int, bool>& assignment);
        
    public:
        d_DNNF(model_counter* mc);
        ~d_DNNF();
        
        // 将DAG转换为d-DNNF
        bool convert_to_dnnf(dagc* dag_root);
        
        // 在d-DNNF上进行model counting
        long long count_models();
        
        // 获取所有使用的变量
        std::vector<int> get_variables();
        
        // 输出d-DNNF结构（调试用）
        void print_dnnf(std::shared_ptr<DNNFNode> node = nullptr, int depth = 0);
        
        // 验证d-DNNF的决策性和分解性
        bool verify_dnnf_properties();
        
        // 获取d-DNNF根节点
        std::shared_ptr<DNNFNode> get_root() { return root; }
    };
}

#endif

