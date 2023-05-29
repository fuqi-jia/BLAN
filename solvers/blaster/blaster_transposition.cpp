/* blaster_transposition.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_solver.hpp"

using namespace ismt;


// Transposition Abstraction: e.g. a > b <=> a - b > 0
literal blaster_solver::TEqual(bvar var1, bvar var2){
    bvar ans = Subtract(var1, var2);
    return EqZero(ans);
}
literal blaster_solver::TNotEqual(bvar var1, bvar var2){
    bvar ans = Subtract(var1, var2);
    return NeqZero(ans);
}
literal blaster_solver::TGreater(bvar var1, bvar var2){
    bvar ans = Subtract(var1, var2);
    return GtZero(ans);
}
literal blaster_solver::TGreaterEqual(bvar var1, bvar var2){
    bvar ans = Subtract(var1, var2);
    return GeZero(ans);
}
literal blaster_solver::TEqualInt(bvar var, bvar v){
    if(var->isConstant() && var->getCurValue()==0) return EqZero(v);
    else if(v->isConstant() && v->getCurValue()==0) return EqZero(var); 
    bvar ans = Subtract(var, v);
    return EqZero(ans);
}
literal blaster_solver::TNotEqualInt(bvar var, bvar v){
    if(var->isConstant() && var->getCurValue()==0) return NeqZero(v);
    else if(v->isConstant() && v->getCurValue()==0) return NeqZero(var); 
    bvar ans = Subtract(var, v);
    return NeqZero(ans);
}
literal blaster_solver::TGreaterInt(bvar var, bvar v){
    if(var->isConstant() && var->getCurValue() == 0) return LtZero(v);
    else if(v->isConstant() && v->getCurValue() == 0) return GtZero(var);
    bvar ans = Subtract(var, v);
    return GtZero(ans);
}
literal blaster_solver::TGreaterEqualInt(bvar var, bvar v){
    if(var->isConstant() && var->getCurValue() == 0) return LeZero(v);
    else if(v->isConstant() && v->getCurValue() == 0) return GeZero(var);
    bvar ans = Subtract(var, v);
    return GeZero(ans);
}