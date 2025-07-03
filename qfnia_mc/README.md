# QF_NIA Model Counter

这是一个基于libpoly和现有QF_NIA求解器框架实现的模型计数器，专门用于量词无关的非线性整数运算（Quantifier-Free Nonlinear Integer Arithmetic）公式的模型计数。

## 功能特性

### 核心算法
- **d-DNNF转换**: 将DAG结构的公式转换为决策分解神经网络形式（Deterministic Decomposable Negation Normal Form）
- **CAD分解**: 使用柱状代数分解（Cylindrical Algebraic Decomposition）计算解空间的元胞数量
- **Bit-blasting**: 将整数约束转换为位级别的布尔约束
- **Projected Model Counting**: 调用ganak求解器进行精确的模型计数

### 技术集成
- 基于libpoly的多项式操作和CAD分解
- 集成CaDiCaL SAT求解器进行布尔可满足性检查
- 支持ganak模型计数器进行精确计数
- 提供多种备用方法确保鲁棒性

## 文件结构

```
qfnia_mc/
├── model_counter.hpp          # 主要接口头文件
├── model_counter.cpp          # 主要实现文件
├── cad.hpp                    # CAD分解头文件
├── cad.cpp                    # CAD分解实现
├── d_DNNF.hpp                 # d-DNNF转换头文件
├── d_DNNF.cpp                 # d-DNNF转换实现
├── example_usage.cpp          # 使用示例
└── README.md                  # 本文件
```

## 核心类说明

### model_counter
主要的模型计数器类，协调整个计数过程。

**主要方法：**
- `init(Info* info)`: 初始化计数器
- `count_model()`: 执行模型计数
- `convert_to_dnnf_and_count()`: d-DNNF方法
- `perform_cad_decomposition()`: CAD分解方法
- `perform_bit_blasting(int cad_cells)`: bit-blasting方法
- `call_ganak_solver()`: 调用ganak求解器

### cad
CAD分解实现类，基于libpoly。

**主要方法：**
- `count_model(dagc* root)`: 对给定约束执行CAD分解并计算元胞数量

### d_DNNF  
d-DNNF转换和模型计数类。

**主要方法：**
- `convert_to_dnnf(dagc* dag_root)`: 将DAG转换为d-DNNF
- `count_models()`: 在d-DNNF上执行模型计数
- `verify_dnnf_properties()`: 验证d-DNNF的决策性和分解性

## 算法流程

```
输入：QF_NIA公式（DAG形式）
│
├─ 步骤1：d-DNNF转换
│  ├─ 将DAG转换为d-DNNF形式
│  ├─ 验证决策性和分解性
│  └─ 在d-DNNF上执行模型计数
│
├─ 步骤2：CAD分解（d-DNNF失败时）
│  ├─ 提取多项式约束
│  ├─ 执行投影操作
│  ├─ 计算CAD元胞数量
│  └─ 返回元胞数作为模型估计
│
├─ 步骤3：Bit-blasting
│  ├─ 将整数变量转换为位向量
│  ├─ 将算术约束转换为布尔约束
│  └─ 使用SAT求解器检查可满足性
│
└─ 步骤4：Ganak模型计数
   ├─ 生成DIMACS CNF格式
   ├─ 调用ganak求解器
   ├─ 解析输出结果
   └─ 返回精确的模型数量
```

## 使用方法

### 基本用法

```cpp
#include "model_counter.hpp"

// 1. 创建model_counter实例
auto mc = std::make_unique<model_counter>();

// 2. 初始化（需要Info对象）
Info info = setup_info(); // 设置必要的组件
mc->init(&info);

// 3. 执行模型计数
int count = mc->count_model();
std::cout << "模型数量: " << count << std::endl;
```

### 完整示例

参见 `example_usage.cpp` 文件，其中包含了一个完整的使用示例，演示如何：
- 构建DAG约束
- 创建必要的组件（Parser, Collector, Preprocessor等）
- 执行模型计数
- 处理结果

## 编译要求

### 依赖库
1. **libpoly**: 多项式操作和CAD分解
2. **CaDiCaL**: SAT求解器
3. **ganak** (可选): 精确模型计数器
4. **C++17**: 编译器支持

### 编译命令示例

```bash
g++ -std=c++17 -I. -I./solvers/include \
    example_usage.cpp model_counter.cpp cad.cpp d_DNNF.cpp \
    -lpoly -lcadical -o model_counter_example
```

## 配置选项

可以通过修改 `model_counter.cpp` 中的 `#define mcDebug 1` 来启用调试输出。

## 性能特点

### 优势
- **多层次备用**: 如果某种方法失败，会自动尝试其他方法
- **精确性**: 结合多种精确算法（d-DNNF、CAD、ganak）
- **鲁棒性**: 提供近似估计作为最后备用方案

### 复杂度
- **d-DNNF**: 取决于公式结构，最好情况下线性时间
- **CAD**: 双指数复杂度，但对某些问题非常有效
- **Bit-blasting**: 指数级别，但SAT求解器有很好的实际性能

## 限制和注意事项

1. **变量域**: 目前主要支持整数域
2. **约束类型**: 专门针对QF_NIA约束
3. **内存使用**: CAD分解和bit-blasting可能消耗大量内存
4. **时间复杂度**: 对于复杂约束可能需要很长时间

## 示例测试用例

```smt2
; 圆形区域约束
(declare-fun x () Int)
(declare-fun y () Int)
(assert (<= (+ (* x x) (* y y)) 4))
```

这个约束的整数解包括：(0,0), (±1,0), (0,±1), (±2,0), (0,±2) 等点。

## 扩展建议

1. **支持实数域**: 扩展到QF_NRA
2. **优化算法**: 针对特定类型的约束进行优化
3. **并行化**: 利用多核处理器并行执行不同的算法
4. **缓存机制**: 缓存中间结果以加速重复计算

## 故障排除

### 常见问题
1. **编译错误**: 确保所有依赖库已正确安装
2. **运行时错误**: 检查输入约束的格式是否正确
3. **性能问题**: 考虑简化约束或使用近似方法

### 调试建议
- 启用 `mcDebug` 查看详细的执行过程
- 检查临时文件（如 `temp_formula.cnf`）的内容
- 使用简单的测试用例验证功能

## 作者和许可

Copyright (C) 2025 Fuqi Jia. All rights reserved.

基于现有的QF_NIA求解器框架和libpoly库开发。 