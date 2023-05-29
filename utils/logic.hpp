/* logic.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _LOGIC_H
#define _LOGIC_H

namespace ismt
{
    
    enum ENUM_LOGIC {
        UNKNOWN_LOGIC,
        QF_LIA,
        QF_NIA,
        QF_LRA,
        QF_LIRA,
        QF_NRA,
        QF_NIRA
    };

    enum NODE_TYPE {
        NT_UNKNOWN=0, NT_ERROR,
        // CONST
        NT_CONST_BOOL, NT_CONST_NUM,
        // VAR
        NT_VAR_BOOL, NT_VAR_NUM,
        // LOGIC
        NT_AND, NT_OR, NT_NOT, NT_ITE_BOOL, NT_ITE_NUM,
        // ARITHMATIC
        NT_ADD, NT_MUL, NT_DIV_INT, NT_DIV_REAL, NT_MOD, NT_ABS, NT_SUB, 
        // COMP
        NT_LE, NT_EQ, NT_LT, NT_GE, NT_GT, NT_NEQ, NT_EQ_BOOL, NT_NEQ_BOOL, 
        // INT REAL
        NT_TO_REAL, NT_TO_INT, NT_IS_INT,
        // LET 
        NT_VAR_LET_BOOL, NT_VAR_LET_NUM, NT_LET, 
        // DEFINE-FUN 
        NT_FUN_BOOL, NT_FUN_INT, NT_FUN_REAL, NT_FUN_PARAM_BOOL, NT_FUN_PARAM_INT
    };
} // namespace ismt



#endif