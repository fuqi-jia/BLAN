/* cad.hpp
*
*  Copyright (C) 2025 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _CAD_HPP
#define _CAD_HPP

#include "model_counter.hpp"

namespace ismt
{
    class cad
    {
    private:
        model_counter* mc = nullptr;
    public:
        cad(model_counter* mc);
        ~cad();
        int count_model(dagc* root);
    };
}

#endif