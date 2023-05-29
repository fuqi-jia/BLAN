/* blaster_offset.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _BLASTER_OFFSET_H
#define _BLASTER_OFFSET_H

#include "utils/types.hpp"
#include "utils/interval.hpp"
#include "solvers/blaster/blaster_bits.hpp"

namespace ismt
{
    // if isDefault, blast(bits) -> var
    // else 
    //  if isLower, bound + blast(bits) -> var
    //  else, bound - blast(bits) -> var 
    struct offset
    {
        Integer bound;
        Integer width;
        bool isDefault;
        bool isLower;
        bool isSat;
        unsigned bits;
        offset(const Interval& interval, unsigned defaultLen){
            if(isFull(interval)){
                // (-oo, +oo)
                bound = 0;
                width = -1;
                isDefault = true;
                isLower = false;
                isSat = true;
                bits = defaultLen;
            }
            else if( isPoint(interval)){
                // [lower, lower]
                bound = poly::to_int(getLower(interval));
                width = 0;
                isDefault = false;
                isLower = true;
                isSat = true;
                bits = 0;
            }
            // else if(interval.isEmpty()){
            //     // [lower, lower]
            //     bound = getLower(interval);
            //     width = -1;
            //     isDefault = false;
            //     isLower = true;
            //     isSat = false;
            //     bits = 0;
            // }
            else if(ninf(interval)){
                // (-oo, upper]
                bound = poly::to_int(getUpper(interval));
                width = -1;
                isDefault = false;
                isLower = false;
                isSat = true;
                bits = defaultLen;
            }
            else if(pinf(interval)){
                // [lower, +oo)
                bound = poly::to_int(getLower(interval));
                width = -1;
                isDefault = false;
                isLower = true;
                isSat = true;
                bits = defaultLen;
            }
            else{
                // [lower, upper]
                bound = poly::to_int(getLower(interval));
                width = poly::to_int(getUpper(interval) - getLower(interval));
                isDefault = false;
                isLower = true;
                isSat = true;
                bits = blastBitLength( poly::to_int(getUpper(interval) - getLower(interval)) );
            }
        }
    };
} // namespace ismt


#endif