/* example_usage.cpp
 *
 * 这是一个展示如何使用model_counter进行QF_NIA模型计数的示例程序
 *
 * Copyright (C) 2025 Fuqi Jia.
 * All rights reserved.
 */

#include "model_counter.hpp"
#include "frontend/parser.hpp"
#include "midend/preprocessor.hpp"
#include "qfnia/info.hpp"
#include "utils/model.hpp"
#include <iostream>
#include <memory>

using namespace ismt;

int main() {
    std::cout << "=======================================" << std::endl;
    std::cout << "QF_NIA Model Counter 示例程序" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    try {
        // 1. 创建model_counter实例
        auto mc = std::make_unique<model_counter>();
        std::cout << "✓ Model Counter创建成功" << std::endl;
        
        // 2. 创建简单的测试用例
        // 这里我们创建一个简单的约束：x^2 + y^2 <= 4 (圆形区域)
        
        // 创建DAG结构
        DAG test_dag;
        
        // 创建模型
        auto model = std::make_unique<model_s>();
        
        // 创建解析器（简化版本，实际使用时需要从文件读取）
        // 这里我们手动构建一个简单的约束
        std::cout << "✓ 构建测试约束：x^2 + y^2 <= 4" << std::endl;
        
        // 模拟解析过程的结果
        // 在实际应用中，这些将通过Parser从SMT-LIB2文件中解析得到
        
        // 创建变量
        dagc* x_var = new dagc();
        x_var->t = NT_VNUM;
        x_var->name = "x";
        x_var->setVarType(INT_VAR);
        test_dag.vnum_int_list.push_back(x_var);
        
        dagc* y_var = new dagc();
        y_var->t = NT_VNUM;
        y_var->name = "y";
        y_var->setVarType(INT_VAR);
        test_dag.vnum_int_list.push_back(y_var);
        
        // 创建约束：x^2 + y^2 <= 4
        dagc* constraint = new dagc();
        constraint->t = NT_LE;
        constraint->name = "<=";
        
        // 左侧：x^2 + y^2
        dagc* add_node = new dagc();
        add_node->t = NT_ADD;
        add_node->name = "+";
        
        dagc* x_squared = new dagc();
        x_squared->t = NT_MUL;
        x_squared->name = "*";
        x_squared->children.push_back(x_var);
        x_squared->children.push_back(x_var);
        
        dagc* y_squared = new dagc();
        y_squared->t = NT_MUL;
        y_squared->name = "*";
        y_squared->children.push_back(y_var);
        y_squared->children.push_back(y_var);
        
        add_node->children.push_back(x_squared);
        add_node->children.push_back(y_squared);
        
        // 右侧：常数4
        dagc* const_4 = new dagc();
        const_4->t = NT_CNUM;
        const_4->name = "4";
        const_4->v = 4;
        
        constraint->children.push_back(add_node);
        constraint->children.push_back(const_4);
        
        test_dag.assert_list.push_back(constraint);
        
        std::cout << "✓ 测试DAG构建完成" << std::endl;
        
        // 3. 创建必要的组件
        Parser dummy_parser("", test_dag);
        Collector collector(&dummy_parser, model.get());
        preprocessor prep(&dummy_parser, &collector, model.get());
        
        // 设置模型变量
        model->setVariables(test_dag.vbool_list);
        model->setVariables(test_dag.vnum_int_list);
        
        std::cout << "✓ 预处理器组件创建完成" << std::endl;
        
        // 4. 创建Info对象
        SolverOptions options;
        options.File = "test_case";
        
        Message* message = new Message();
        message->data = &test_dag;
        message->model = model.get();
        
        Info info(message, &collector, &prep, &options);
        
        std::cout << "✓ Info对象创建完成" << std::endl;
        
        // 5. 初始化model_counter
        mc->init(&info);
        std::cout << "✓ Model Counter初始化完成" << std::endl;
        
        // 6. 执行模型计数
        std::cout << "\n开始执行模型计数..." << std::endl;
        std::cout << "约束：x^2 + y^2 <= 4" << std::endl;
        std::cout << "变量：x, y (整数)" << std::endl;
        
        int model_count = mc->count_model();
        
        std::cout << "\n=======================================" << std::endl;
        std::cout << "模型计数结果：" << model_count << std::endl;
        std::cout << "=======================================" << std::endl;
        
        if (model_count > 0) {
            std::cout << "✓ 模型计数成功完成" << std::endl;
            std::cout << "解释：在整数域中，满足 x^2 + y^2 <= 4 的点有 " 
                      << model_count << " 个" << std::endl;
            std::cout << "理论上应该包括：(0,0), (±1,0), (0,±1), (±1,±1), (±2,0), (0,±2) 等点" << std::endl;
        } else {
            std::cout << "✗ 模型计数失败或约束不可满足" << std::endl;
        }
        
        // 清理资源
        delete message;
        
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n示例程序执行完成！" << std::endl;
    return 0;
}

/* 编译说明：
 * 
 * 要编译此示例程序，请确保：
 * 1. 已安装libpoly库
 * 2. 已安装CaDiCaL SAT求解器
 * 3. （可选）已安装ganak模型计数器
 * 
 * 编译命令示例：
 * g++ -std=c++17 -I. -I./solvers/include \
 *     example_usage.cpp model_counter.cpp cad.cpp d_DNNF.cpp \
 *     -lpoly -lcadical -o model_counter_example
 * 
 * 运行：
 * ./model_counter_example
 */ 