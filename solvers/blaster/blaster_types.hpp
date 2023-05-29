/* blaster_types.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _BLASTER_TYPES_H
#define _BLASTER_TYPES_H


#include "utils/types.hpp"

#include <vector>
#include <string>
#include <iostream>

namespace ismt
{
    const int mask_constant = 1;
    const int mask_clear = 2;
    const int mask_zero = 4;
    class blast_variable
    {
    private:
        int                 info;
        Integer             curValue;
        std::vector<int>    data;
        std::string         name;
    public:
        blast_variable(const std::string& n, unsigned len, Integer v=0, bool isc=false){
            setName(n);
            setSize(len); 
            setCurValue(v);
            setKind(isc);
            setZero(); // default has zero.
        }
        ~blast_variable(){}
        
        // getter 
        int         signBit()           const { return data[0]; }
        bool        isConstant()        const { return info & mask_constant; }
        bool        isClean()           const { return info & mask_clear; }
        bool        hasZero()           const { return info & mask_zero; }
        unsigned    size()              const { return data.size(); }
        unsigned    csize()             const { return data.size()-1; } // content size
        Integer     getCurValue()       const { return curValue; }
        int         getAt(unsigned i)   const { return data[i]; }
        std::string getName()           const { return name; }

        // setter
        void        setKind(bool isc)               { if(isc) info |= mask_constant; else info &= ~mask_constant; }
        void        setSize(unsigned len)           { data.resize(len+1); } // for sign bit
        void        setCurValue(Integer v)          { curValue = v; }
        void        setAt(size_t i, int v)          { data[i] = v; }
        void        setName(const std::string& n)   { name = n; }
        void        setZero()                       { info |= mask_zero; }
        void        unsetZero()                     { info &= ~mask_zero; }

        // friend operations
        void        reblast()                       { info &= ~mask_clear; }
        void        clear()                         { data.clear(); info |= mask_clear; } // NEW
        // when staying at formula tree, once restart this of variable kind can be clear. 

    };

    typedef blast_variable* bvar;
    
} // namespace ismt


#endif