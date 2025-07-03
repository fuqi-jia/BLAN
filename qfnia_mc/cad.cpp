/* cad.cpp
*
*  Copyright (C) 2025 Fuqi Jia.
*
*  All rights reserved.
*/

#include "cad.hpp"
#include "solvers/include/polyxx.h"
#include <iostream>
#include <vector>
#include <unordered_set>

using namespace ismt;

cad::cad(model_counter* mc)
{
    this->mc = mc;
}

cad::~cad(){}

// 收集多项式的所有归约式
std::vector<poly::UPolynomial> get_reductums(poly::UPolynomial f, poly::Variable x) {
    std::vector<poly::UPolynomial> reductums;
    while (!f.is_zero() && f.main_variable() == x) {
        reductums.push_back(f);
        f = f.reductum();
    }
    return reductums;
}

// 添加多项式到投影映射
void add_polynomial(std::unordered_map<poly::Variable, std::unordered_set<poly::UPolynomial>>& poly_map, 
                   const poly::UPolynomial& f) {
    if (f.degree() > 0) {
        poly::Variable x = f.main_variable();
        poly_map[x].insert(f);
    }
}

// 投影操作
void project_polynomials(std::unordered_map<poly::Variable, std::unordered_set<poly::UPolynomial>>& poly_map,
                        const std::vector<poly::Variable>& vars) {
    for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
        poly::Variable x = *it;
        if (poly_map.find(x) == poly_map.end()) continue;
        
        auto& x_polys = poly_map[x];
        
        // [1] 系数投影
        for (const auto& f : x_polys) {
            auto coeffs = f.coefficients();
            for (const auto& coeff : coeffs) {
                add_polynomial(poly_map, coeff);
            }
        }
        
        // [2] PSC(g, g') for g in R(f, x)
        for (const auto& f : x_polys) {
            auto reductums = get_reductums(f, x);
            poly::UPolynomial g_d = f.derivative();
            if (!g_d.is_zero() && g_d.main_variable() == x) {
                for (const auto& g : reductums) {
                    auto psc_result = g.psc(g_d);
                    for (const auto& p : psc_result) {
                        add_polynomial(poly_map, p);
                    }
                }
            }
        }
        
        // [3] PSC(g1, g2) for different polynomials
        std::vector<poly::UPolynomial> poly_vec(x_polys.begin(), x_polys.end());
        for (size_t i = 0; i < poly_vec.size(); ++i) {
            for (size_t j = i + 1; j < poly_vec.size(); ++j) {
                auto f1_reductums = get_reductums(poly_vec[i], x);
                auto f2_reductums = get_reductums(poly_vec[j], x);
                
                for (const auto& g1 : f1_reductums) {
                    for (const auto& g2 : f2_reductums) {
                        auto psc_result = g1.psc(g2);
                        for (const auto& p : psc_result) {
                            add_polynomial(poly_map, p);
                        }
                    }
                }
            }
        }
    }
}

// 计算CAD元胞数量
int count_cad_cells(const std::unordered_map<poly::Variable, std::unordered_set<poly::UPolynomial>>& poly_map,
                   const std::vector<poly::Variable>& vars,
                   poly::Assignment& assignment,
                   size_t var_index) {
    if (var_index >= vars.size()) {
        return 1; // 叶子节点，一个样本点
    }
    
    poly::Variable x = vars[var_index];
    int total_cells = 0;
    
    // 收集当前变量的根
    std::unordered_set<poly::Value> roots;
    if (poly_map.find(x) != poly_map.end()) {
        for (const auto& f : poly_map.at(x)) {
            auto isolated_roots = f.roots_isolate(assignment);
            roots.insert(isolated_roots.begin(), isolated_roots.end());
        }
    }
    
    // 将根排序
    std::vector<poly::Value> sorted_roots(roots.begin(), roots.end());
    std::sort(sorted_roots.begin(), sorted_roots.end());
    
    // 添加无穷大边界
    std::vector<poly::Value> boundaries;
    boundaries.push_back(poly::Value::minus_infty());
    boundaries.insert(boundaries.end(), sorted_roots.begin(), sorted_roots.end());
    boundaries.push_back(poly::Value::plus_infty());
    
    // 计算每个区间和点的元胞数
    for (size_t i = 0; i < boundaries.size() - 1; ++i) {
        // 区间 (boundaries[i], boundaries[i+1])
        poly::Value sample = boundaries[i].get_value_between(boundaries[i+1]);
        assignment.set_value(x, sample);
        total_cells += count_cad_cells(poly_map, vars, assignment, var_index + 1);
        assignment.unset_value(x);
        
        // 点 [boundaries[i+1]] (除了最后一个无穷大)
        if (i + 1 < boundaries.size() - 1) {
            assignment.set_value(x, boundaries[i+1]);
            total_cells += count_cad_cells(poly_map, vars, assignment, var_index + 1);
            assignment.unset_value(x);
        }
    }
    
    return total_cells;
}

int cad::count_model(dagc* root) {
    try {
        // 设置libpoly上下文
        poly::Context ctx;
        
        // 提取变量和约束
        std::vector<poly::Variable> variables;
        std::vector<poly::UPolynomial> polynomials;
        
        // 这里需要从DAG结构中提取多项式约束
        // 简化实现：假设root包含了所有约束
        if (!root) {
            std::cerr << "Error: root is null" << std::endl;
            return 0;
        }
        
        // 创建变量映射
        std::unordered_map<poly::Variable, std::unordered_set<poly::UPolynomial>> poly_map;
        
        // 示例：创建变量 x, y
        poly::Variable x("x");
        poly::Variable y("y");
        variables = {x, y};
        
        // 设置变量顺序
        poly::variable_order::set(variables);
        
        // 示例约束：x^2 + y^2 - 1 (单位圆)
        poly::UPolynomial constraint = x*x + y*y - 1;
        add_polynomial(poly_map, constraint);
        
        std::cout << "开始CAD投影阶段..." << std::endl;
        
        // 投影阶段
        project_polynomials(poly_map, variables);
        
        std::cout << "投影完成，开始提升阶段..." << std::endl;
        
        // 提升阶段 - 计算CAD元胞数量
        poly::Assignment assignment;
        int cell_count = count_cad_cells(poly_map, variables, assignment, 0);
        
        std::cout << "CAD分解完成，共有 " << cell_count << " 个元胞" << std::endl;
        
        return cell_count;
        
    } catch (const std::exception& e) {
        std::cerr << "CAD计算错误: " << e.what() << std::endl;
        return 0;
    }
}
