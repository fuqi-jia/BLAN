/* box.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _BOX_HPP
#define _BOX_HPP

// now not support, for less memory
#include "utils/interval.hpp"

namespace ismt
{
    const int mask_is_interval = 1;

    class box
    {
    private:
        int info = 0;
        unsigned int bits = 0;
        // Interval 
    public:
        box(int i = 0):info(i){}
        ~box();

        bool is_free()      const { return !(info & mask_is_interval); }
        bool is_interval()  const { return (info & mask_is_interval); }
    };

} // namespace ismt


#endif