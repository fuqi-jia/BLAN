/* blaster_comp.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"

using namespace ismt;


// ------------math atom operations------------

// math atom operations
literal blaster_solver::Less(bvar var1, bvar var2){ // var1 < var2
    if(var1==var2) return solver->False();
    if(var1->isConstant()&&var2->isConstant()){
        return var1->getCurValue() < var2->getCurValue()?solver->True():solver->False();
    }
    assert(!var1->isConstant() || !var2->isConstant());

    if(var1->isConstant() || var2->isConstant()){
        return viGt(var2, var1);
    }
    else{
        return vvGt(var2, var1);
    }
}
literal blaster_solver::Equal(bvar var1, bvar var2){
    if(var1==var2) return solver->True();
    if(var1->isConstant()&&var2->isConstant()){
        return var1->getCurValue() == var2->getCurValue()?solver->True():solver->False();
    }
    assert(!var1->isConstant() || !var2->isConstant());
    
    if(var1->isConstant() || var2->isConstant()){
        return viEq(var1, var2);
    }
    else{
        return vvEq(var1, var2);
    }
}
literal blaster_solver::Greater(bvar var1, bvar var2){ // var1 > var2
    if(var1==var2) return solver->False();
    if(var1->isConstant()&&var2->isConstant()){
        return var1->getCurValue() > var2->getCurValue()?solver->True():solver->False();
    }
    assert(!var1->isConstant() || !var2->isConstant());

    if(var1->isConstant() || var2->isConstant()){
        return viGt(var1, var2);
    }
    else{
        return vvGt(var1, var2);
    }
}
literal blaster_solver::NotEqual(bvar var1, bvar var2){
    if(var1==var2) return solver->False();
    if(var1->isConstant()&&var2->isConstant()){
        return var1->getCurValue() != var2->getCurValue()?solver->True():solver->False();
    }
    assert(!var1->isConstant() || !var2->isConstant());

    if(var1->isConstant() || var2->isConstant()){
        return viNeq(var1, var2);
    }
    else{
        return vvNeq(var1, var2);
    }
}
literal blaster_solver::LessEqual(bvar var1, bvar var2){ // var1 <= var2
    if(var1==var2) return solver->True();
    if(var1->isConstant()&&var2->isConstant()){
        return var1->getCurValue() <= var2->getCurValue()?solver->True():solver->False();
    }
    assert(!var1->isConstant() || !var2->isConstant());

    if(var1->isConstant() || var2->isConstant()){
        return viGe(var2, var1);
    }
    else{
        return vvGe(var2, var1);
    }
}
literal blaster_solver::GreaterEqual(bvar var1, bvar var2){
    if(var1==var2) return solver->True();
    if(var1->isConstant()&&var2->isConstant()){
        return var1->getCurValue() >= var2->getCurValue()?solver->True():solver->False();
    }
    assert(!var1->isConstant() || !var2->isConstant());
    
    if(var1->isConstant() || var2->isConstant()){
        return viGe(var1, var2);
    }
    else{
        return vvGe(var1, var2);
    }
    
}
