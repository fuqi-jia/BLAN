/* checker.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _CHECKER_HPP
#define _CHECKER_HPP

#include "utils/model.hpp"
#include "qfnia/info.hpp"

namespace ismt
{   
    // model checker
    class checker
    {
    private:
        Info*       info = nullptr;
        // check
        int         checkAtoms(dagc* dag);
        int         checkMathAtoms(dagc* dag);
        int         checkLetBoolTerms(dagc* dag);
        int         checkLetAtoms(dagc* dag);
        Integer     checkIteNums(dagc* dag);
        Integer     checkMathTerms(dagc* dag);
        Integer     checkLetNumTerms(dagc* dag);
        Integer     getValue(dagc* dag);
        bool        check(dagc* dag);
    public:
        checker(Info* i):info(i){}
        ~checker(){}

        bool check();
    };
} // namespace ismt


#endif