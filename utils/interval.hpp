/* interval.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _INTERVAL_H
#define _INTERVAL_H

#include "utils/types.hpp"
#include <iostream>

namespace ismt
{
    typedef poly::Interval Interval;
    const Interval FullInterval(Value::minus_infty(), Value::plus_infty());
    const Interval EmptyInterval(0, 0);
    inline bool ninf(const Interval& a){
        return poly::is_minus_infinity(poly::get_lower(a));
    }
    inline bool pinf(const Interval& b){
        return poly::is_plus_infinity(poly::get_upper(b));
    }
    inline bool isFull(const Interval& i){
        return poly::is_full(i);
    }
    inline bool isPoint(const Interval& i){
        return poly::is_point(i);
    }
    // inline bool isEmpty(const Interval& i){
        
    //     return false;
    // }
    inline poly::Integer getUpper(const Interval& i){
        return to_Integer(poly::get_upper(i));
    }
    inline poly::Integer getLower(const Interval& i){
        // std::cout<<i<<" "<<poly::get_lower(i)<<std::endl;
        return to_Integer(poly::get_lower(i));
    }
    inline poly::Integer getRealUpper(const Interval& i){
        return poly::get_upper_open(i)?poly::Integer(poly::to_int(getUpper(i))-1):to_Integer(poly::get_upper(i));
    }
    inline poly::Integer getRealLower(const Interval& i){
        return poly::get_lower_open(i)?poly::Integer(poly::to_int(getLower(i))+1):to_Integer(poly::get_lower(i));
    }
    inline bool overlap(const Interval& a, const Interval& b){
        if( !pinf(a) && !ninf(b) && getRealUpper(a) < getRealLower(b)) return false; // (-oo, 3] and [5, +oo).
        if( !pinf(b) && !ninf(a) && getRealLower(a) > getRealUpper(b)) return false; // [0, 3] and [-3, -2].
        return true;
    }
    inline bool has(const Interval& i, const poly::Integer& k){
        return (
            ( ninf(i) && pinf(i) ) ||
            ( ninf(i) && getUpper(i) >= k ) ||
            ( pinf(i) && getLower(i) <= k ) ||
            ( getLower(i) <= k && getUpper(i) >= k )
        );
    }
    inline poly::Integer get_inner_value(const Interval& i, bool& flag){
        flag = true;
        if(isFull(i)){
            return poly::Integer(0);
        }
        else if(ninf(i)){
            poly::Integer u = getRealUpper(i);
            return u;
        }
        else if(pinf(i)){
            poly::Integer l = getRealLower(i);
            return l;
        }
        poly::Integer l = getRealLower(i);
        poly::Integer u = getRealUpper(i);
        poly::Integer One = poly::Integer(1);

        if(l > u){
            flag = false;
            return poly::Integer(0);
        }
        else if(l == u){
            return l;
        }
        else{
            return l + One;
        }
    }
} // namespace ismt


#endif