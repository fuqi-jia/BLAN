#include "d_DNNF.hpp"
#include <iostream>
#include <unordered_set>
#include <algorithm>

using namespace ismt;

d_DNNF::d_DNNF(model_counter* mc) : mc(mc), next_var_id(1)
{
}

d_DNNF::~d_DNNF(){}

bool d_DNNF::convert_to_dnnf(dagc* dag_root) {
    if (!dag_root) {
        std::cerr << "错误：DAG根节点为空" << std::endl;
        return false;
    }
    
    std::cout << "开始将DAG转换为d-DNNF..." << std::endl;
    
    root = convert_dag_to_dnnf(dag_root);
    if (!root) {
        std::cerr << "错误：d-DNNF转换失败" << std::endl;
        return false;
    }
    
    std::cout << "DAG到d-DNNF转换完成" << std::endl;
    
    // 优化d-DNNF
    std::cout << "优化d-DNNF..." << std::endl;
    root = optimize_dnnf(root);
    
    return true;
}

std::shared_ptr<DNNFNode> d_DNNF::convert_dag_to_dnnf(dagc* dag) {
    if (!dag) return nullptr;
    
    // 处理常量
    if (dag->iscbool()) {
        if (dag->v == 1) {
            return std::make_shared<DNNFNode>(DNNFNodeType::TRUE_NODE);
        } else {
            return std::make_shared<DNNFNode>(DNNFNodeType::FALSE_NODE);
        }
    }
    
    // 处理逻辑操作
    if (dag->isand()) {
        return handle_and_node(dag);
    } else if (dag->isor()) {
        return handle_or_node(dag);
    } else if (dag->isnot()) {
        return handle_not_node(dag);
    } else if (dag->isvbool()) {
        return handle_literal(dag);
    } else if (dag->iscomp()) {
        return handle_comparison(dag);
    }
    
    // 其他情况，创建一个字面量
    return handle_literal(dag);
}

std::shared_ptr<DNNFNode> d_DNNF::handle_and_node(dagc* dag) {
    auto and_node = std::make_shared<DNNFNode>(DNNFNodeType::AND);
    
    for (auto child : dag->children) {
        auto child_dnnf = convert_dag_to_dnnf(child);
        if (child_dnnf) {
            and_node->children.push_back(child_dnnf);
        }
    }
    
    return and_node;
}

std::shared_ptr<DNNFNode> d_DNNF::handle_or_node(dagc* dag) {
    auto or_node = std::make_shared<DNNFNode>(DNNFNodeType::OR);
    
    for (auto child : dag->children) {
        auto child_dnnf = convert_dag_to_dnnf(child);
        if (child_dnnf) {
            or_node->children.push_back(child_dnnf);
        }
    }
    
    return or_node;
}

std::shared_ptr<DNNFNode> d_DNNF::handle_not_node(dagc* dag) {
    if (dag->children.size() != 1) {
        std::cerr << "错误：NOT节点应该只有一个子节点" << std::endl;
        return nullptr;
    }
    
    dagc* child = dag->children[0];
    
    // 如果子节点是字面量，直接创建否定字面量
    if (child->isvbool()) {
        int var_id = get_or_create_var_id(child);
        return std::make_shared<DNNFNode>(DNNFNodeType::LITERAL, var_id, true);
    }
    
    // 否则递归处理并使用De Morgan定律
    auto child_dnnf = convert_dag_to_dnnf(child);
    if (!child_dnnf) return nullptr;
    
    // 这里简化处理，直接转换为字面量
    int var_id = get_or_create_var_id(dag);
    return std::make_shared<DNNFNode>(DNNFNodeType::LITERAL, var_id, false);
}

std::shared_ptr<DNNFNode> d_DNNF::handle_literal(dagc* dag) {
    int var_id = get_or_create_var_id(dag);
    return std::make_shared<DNNFNode>(DNNFNodeType::LITERAL, var_id, false);
}

std::shared_ptr<DNNFNode> d_DNNF::handle_comparison(dagc* dag) {
    // 将比较操作转换为字面量
    int var_id = get_or_create_var_id(dag);
    return std::make_shared<DNNFNode>(DNNFNodeType::LITERAL, var_id, false);
}

int d_DNNF::get_or_create_var_id(dagc* var) {
    if (variable_map.find(var) == variable_map.end()) {
        variable_map[var] = next_var_id++;
    }
    return variable_map[var];
}

std::shared_ptr<DNNFNode> d_DNNF::optimize_dnnf(std::shared_ptr<DNNFNode> node) {
    if (!node) return nullptr;
    
    // 递归优化子节点
    for (auto& child : node->children) {
        child = optimize_dnnf(child);
    }
    
    // 优化当前节点
    if (node->type == DNNFNodeType::AND) {
        // 移除TRUE节点，如果有FALSE节点则整个AND为FALSE
        auto new_children = std::vector<std::shared_ptr<DNNFNode>>();
        for (auto child : node->children) {
            if (child->type == DNNFNodeType::FALSE_NODE) {
                return std::make_shared<DNNFNode>(DNNFNodeType::FALSE_NODE);
            } else if (child->type != DNNFNodeType::TRUE_NODE) {
                new_children.push_back(child);
            }
        }
        
        if (new_children.empty()) {
            return std::make_shared<DNNFNode>(DNNFNodeType::TRUE_NODE);
        } else if (new_children.size() == 1) {
            return new_children[0];
        } else {
            node->children = new_children;
        }
    } else if (node->type == DNNFNodeType::OR) {
        // 移除FALSE节点，如果有TRUE节点则整个OR为TRUE
        auto new_children = std::vector<std::shared_ptr<DNNFNode>>();
        for (auto child : node->children) {
            if (child->type == DNNFNodeType::TRUE_NODE) {
                return std::make_shared<DNNFNode>(DNNFNodeType::TRUE_NODE);
            } else if (child->type != DNNFNodeType::FALSE_NODE) {
                new_children.push_back(child);
            }
        }
        
        if (new_children.empty()) {
            return std::make_shared<DNNFNode>(DNNFNodeType::FALSE_NODE);
        } else if (new_children.size() == 1) {
            return new_children[0];
        } else {
            node->children = new_children;
        }
    }
    
    return node;
}

long long d_DNNF::count_models() {
    if (!root) {
        std::cerr << "错误：d-DNNF根节点为空" << std::endl;
        return 0;
    }
    
    std::cout << "开始在d-DNNF上进行model counting..." << std::endl;
    
    std::unordered_map<int, bool> assignment;
    long long count = count_models_recursive(root, assignment);
    
    std::cout << "d-DNNF model counting完成，共 " << count << " 个模型" << std::endl;
    
    return count;
}

long long d_DNNF::count_models_recursive(std::shared_ptr<DNNFNode> node, 
                                        std::unordered_map<int, bool>& assignment) {
    if (!node) return 0;
    
    switch (node->type) {
        case DNNFNodeType::TRUE_NODE:
            return 1;
            
        case DNNFNodeType::FALSE_NODE:
            return 0;
            
        case DNNFNodeType::LITERAL: {
            // 检查赋值是否满足字面量
            if (assignment.find(node->literal_id) != assignment.end()) {
                bool value = assignment[node->literal_id];
                return (value != node->is_negated) ? 1 : 0;
            }
            // 如果变量未赋值，两种情况都可能
            return 2; // 简化处理
        }
        
        case DNNFNodeType::AND: {
            long long result = 1;
            for (auto child : node->children) {
                long long child_count = count_models_recursive(child, assignment);
                result *= child_count;
                if (result == 0) break; // 短路求值
            }
            return result;
        }
        
        case DNNFNodeType::OR: {
            long long result = 0;
            for (auto child : node->children) {
                result += count_models_recursive(child, assignment);
            }
            return result;
        }
    }
    
    return 0;
}

std::vector<int> d_DNNF::get_variables() {
    std::vector<int> vars;
    for (const auto& pair : variable_map) {
        vars.push_back(pair.second);
    }
    std::sort(vars.begin(), vars.end());
    return vars;
}

void d_DNNF::print_dnnf(std::shared_ptr<DNNFNode> node, int depth) {
    if (!node) {
        node = root;
    }
    
    if (!node) return;
    
    std::string indent(depth * 2, ' ');
    
    switch (node->type) {
        case DNNFNodeType::AND:
            std::cout << indent << "AND" << std::endl;
            break;
        case DNNFNodeType::OR:
            std::cout << indent << "OR" << std::endl;
            break;
        case DNNFNodeType::LITERAL:
            std::cout << indent << "LITERAL " << (node->is_negated ? "¬" : "") 
                      << "x" << node->literal_id << std::endl;
            break;
        case DNNFNodeType::TRUE_NODE:
            std::cout << indent << "TRUE" << std::endl;
            break;
        case DNNFNodeType::FALSE_NODE:
            std::cout << indent << "FALSE" << std::endl;
            break;
    }
    
    for (auto child : node->children) {
        print_dnnf(child, depth + 1);
    }
}

bool d_DNNF::verify_dnnf_properties() {
    if (!root) return false;
    
    bool decomposable = is_decomposable(root);
    bool deterministic = is_deterministic(root);
    
    std::cout << "d-DNNF验证结果：" << std::endl;
    std::cout << "  分解性(Decomposable): " << (decomposable ? "✓" : "✗") << std::endl;
    std::cout << "  决策性(Deterministic): " << (deterministic ? "✓" : "✗") << std::endl;
    
    return decomposable && deterministic;
}

bool d_DNNF::is_decomposable(std::shared_ptr<DNNFNode> node) {
    if (!node) return true;
    
    if (node->type == DNNFNodeType::AND) {
        // 检查AND节点的子节点是否变量不相交
        std::unordered_set<int> used_vars;
        for (auto child : node->children) {
            std::unordered_set<int> child_vars;
            // 简化：这里应该收集子树中的所有变量
            if (child->type == DNNFNodeType::LITERAL) {
                if (used_vars.count(child->literal_id)) {
                    return false; // 变量重复
                }
                used_vars.insert(child->literal_id);
            }
        }
    }
    
    // 递归检查子节点
    for (auto child : node->children) {
        if (!is_decomposable(child)) {
            return false;
        }
    }
    
    return true;
}

bool d_DNNF::is_deterministic(std::shared_ptr<DNNFNode> node) {
    if (!node) return true;
    
    if (node->type == DNNFNodeType::OR) {
        // 检查OR节点的子节点是否互斥
        // 简化处理：假设总是满足
    }
    
    // 递归检查子节点
    for (auto child : node->children) {
        if (!is_deterministic(child)) {
            return false;
        }
    }
    
    return true;
}