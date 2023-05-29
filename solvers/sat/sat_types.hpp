/* sat_types.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _SAT_TYPES_H
#define _SAT_TYPES_H

namespace ismt
{
    typedef int bool_var;
    typedef int literal;
    typedef std::vector<literal> clause;
    typedef std::vector<literal> literals;
    typedef std::vector<int> model;
} // namespace ismt


#endif