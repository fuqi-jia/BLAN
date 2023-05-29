/* blaster_bits.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "solvers/blaster/blaster_bits.hpp"
#include <cassert>
using namespace ismt;


// ------------bits operations------------


// operations related to bit length.
namespace ismt
{
        
    bool isPowerTwo(Integer num){
        return getBitLength(num)!=-1;
    }
    int getBitLength(Integer num){
        for(size_t i=0;i<BitsLen;i++){
            if(num==Bit2Data[i]) return i;
        }
        return -1;
    }
    // the bit length that x/num = 0.
    unsigned divideZero(Integer num){
        for(unsigned i=1;i<BitsLen;i++){
            if(num<Bit2Data[i]) return i-1;
        }
        return BitsLen+1;
    }
    // get x from num = 2^x*p and convert num to p. 
    unsigned compress(Integer& num){
        unsigned cnt = 0;
        while(num%2==0&&num!=0){
            cnt+=1;
            num/=2;
        }
        return cnt;
    }
    unsigned blastBitLength(Integer num){
        // esacpe 2^0 = 1. Start from 2^1 = 2.
        for(unsigned i=1;i<BitsLen;i++){
            if(num<=Bit2Data[i]) return i;
        }
        return BitsLen+1;
    }
    Integer getLowBound(bvar var){
        assert(var->csize()<BitsLen);
        return -Bit2Data[var->csize()];
    }
    Integer getUpBound(bvar var){
        assert(var->csize()<BitsLen);
        return Bit2Data[var->csize()];
    }
} // namespace ismt
