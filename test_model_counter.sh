#!/bin/bash

# Model Counter 测试脚本
# 用于验证配置和安装是否正确

echo "========================================"
echo "Model Counter 完整测试脚本"
echo "========================================"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试结果统计
TESTS_PASSED=0
TESTS_FAILED=0

# 测试函数
test_step() {
    local test_name="$1"
    local test_command="$2"
    
    echo -e "\n${YELLOW}测试: $test_name${NC}"
    
    if eval "$test_command"; then
        echo -e "${GREEN}✓ 通过${NC}"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}✗ 失败${NC}"
        ((TESTS_FAILED++))
        return 1
    fi
}

# 1. 测试基本配置
echo "1. 测试基本依赖配置..."
test_step "配置基本依赖" "./configure.sh"

# 2. 测试Model Counter配置
echo -e "\n2. 测试Model Counter配置..."
test_step "配置Model Counter" "./configure.sh --model-count"

# 3. 检查编译结果
echo -e "\n3. 检查编译结果..."
test_step "检查libcadical" "[ -f ./solvers/include/libcadical.a ]"
test_step "检查libpoly" "[ -f ./solvers/include/libpoly.a ]"
test_step "检查polyxx头文件" "[ -d ./solvers/include/poly ]"

# 4. 检查主程序编译
echo -e "\n4. 检查主程序..."
test_step "检查主程序BLAN" "[ -f ./BLAN ] || make -j4"

# 5. 测试Ganak安装
echo -e "\n5. 测试Ganak安装..."
if command -v nix &> /dev/null; then
    test_step "测试Nix Ganak" "nix shell github:meelgroup/ganak --command ganak --help | head -3"
elif [ -f "./ganak" ]; then
    test_step "测试本地Ganak" "./ganak --help | head -3"
elif command -v ganak &> /dev/null; then
    test_step "测试系统Ganak" "ganak --help | head -3"
else
    echo -e "${YELLOW}⚠ Ganak未安装或不可用${NC}"
    echo "建议运行: ./configure.sh --model-count"
fi

# 6. 测试Model Counter代码编译
echo -e "\n6. 测试Model Counter代码..."
if [ -d "qfnia_mc" ]; then
    echo "检查Model Counter源文件..."
    test_step "model_counter.hpp存在" "[ -f qfnia_mc/model_counter.hpp ]"
    test_step "model_counter.cpp存在" "[ -f qfnia_mc/model_counter.cpp ]"
    test_step "cad.hpp存在" "[ -f qfnia_mc/cad.hpp ]"
    test_step "cad.cpp存在" "[ -f qfnia_mc/cad.cpp ]"
    test_step "d_DNNF.hpp存在" "[ -f qfnia_mc/d_DNNF.hpp ]"
    test_step "d_DNNF.cpp存在" "[ -f qfnia_mc/d_DNNF.cpp ]"
    test_step "example_usage.cpp存在" "[ -f qfnia_mc/example_usage.cpp ]"
    
    # 尝试编译示例程序
    echo -e "\n尝试编译Model Counter示例..."
    COMPILE_CMD="g++ -std=c++17 -I. -I./solvers/include -I./qfnia_mc \
        qfnia_mc/example_usage.cpp qfnia_mc/model_counter.cpp qfnia_mc/cad.cpp qfnia_mc/d_DNNF.cpp \
        -L./solvers/include -lcadical -lpoly -lpolyxx -o model_counter_test 2>/dev/null"
    
    if eval "$COMPILE_CMD"; then
        echo -e "${GREEN}✓ Model Counter示例编译成功${NC}"
        ((TESTS_PASSED++))
        
        # 尝试运行测试
        echo "运行Model Counter测试..."
        if ./model_counter_test 2>/dev/null; then
            echo -e "${GREEN}✓ Model Counter运行测试通过${NC}"
            ((TESTS_PASSED++))
        else
            echo -e "${RED}✗ Model Counter运行测试失败${NC}"
            echo "这可能是由于缺少运行时依赖"
            ((TESTS_FAILED++))
        fi
        
        # 清理测试文件
        rm -f model_counter_test
    else
        echo -e "${RED}✗ Model Counter示例编译失败${NC}"
        echo "请检查依赖库和头文件路径"
        ((TESTS_FAILED++))
    fi
else
    echo -e "${RED}✗ qfnia_mc目录不存在${NC}"
    ((TESTS_FAILED++))
fi

# 7. 生成测试报告
echo -e "\n========================================"
echo "测试报告"
echo "========================================"
echo -e "通过的测试: ${GREEN}$TESTS_PASSED${NC}"
echo -e "失败的测试: ${RED}$TESTS_FAILED${NC}"
echo -e "总测试数: $((TESTS_PASSED + TESTS_FAILED))"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}🎉 所有测试都通过了！${NC}"
    echo "Model Counter已成功配置并可以使用。"
    
    echo -e "\n📖 使用方法："
    echo "1. 在代码中包含: #include \"qfnia_mc/model_counter.hpp\""
    echo "2. 编译时链接: -L./solvers/include -lcadical -lpoly -lpolyxx"
    echo "3. 参考示例: qfnia_mc/example_usage.cpp"
    
    if command -v nix &> /dev/null; then
        echo "4. 使用Ganak: nix shell github:meelgroup/ganak"
    fi
    
    exit 0
else
    echo -e "\n${RED}❌ 部分测试失败${NC}"
    echo "请根据上述错误信息进行修复。"
    
    echo -e "\n🔧 常见问题解决方案："
    echo "1. 编译错误: 确保安装了g++、cmake、make"
    echo "2. 依赖库问题: 重新运行 ./configure.sh"
    echo "3. Ganak问题: 安装nix或手动编译ganak"
    echo "4. 权限问题: 确保当前目录可写"
    
    exit 1
fi 