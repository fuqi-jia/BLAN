/* types.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _TYPES_H
#define _TYPES_H

#include "utils/poly.hpp"
#include <cassert>

namespace ismt
{
    enum class State {UNKNOWN=-1, UNSAT, SAT};
    // typedef poly::Integer Integer;
    typedef poly::Value Value;
    typedef mpz_class Integer;
    typedef mpq_class Rational;

    inline long to_long(const Value& v){
        // assert(poly::is_integer(v));
        poly::Integer i = poly::get_integer(v);
        return poly::to_int(i);
    }
    inline long to_long(const Integer& v){
        // assert(poly::is_integer(v));
        poly::Integer i(v);
        return poly::to_int(i);
    }
    inline Integer to_mpz(const Integer& v){
        // assert(poly::is_integer(v));
        poly::Integer i(v);
        return poly::to_int(i);
    }
    inline poly::Integer to_Integer(const Value& v){
        // assert(poly::is_integer(v));
        poly::Integer i = poly::get_integer(v);
        return i;
    }
    inline poly::Integer to_Integer(const Integer& v){
        poly::Integer i(v);
        return i;
    }
    inline Value to_Value(const Integer& v){
        poly::Integer i(v);
        return i;
    }
    inline Value to_Value(const poly::Integer& v){
        return v;
    }
    inline Value to_Value(const long& v){
        poly::Integer i(v);
        return i;
    }
} // namespace ismt



#endif