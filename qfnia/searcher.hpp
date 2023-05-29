/* searcher.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _SEARCHER_H
#define _SEARCHER_H

#include "qfnia/info.hpp"

#include "solvers/blaster/blaster_solver.hpp"
#include "solvers/blaster/blaster_transformer.hpp"

namespace ismt
{
   class Searcher
   {
   private:
     Info* info = nullptr;
     blaster_transformer* transformer = nullptr;
     blaster_solver* solver = nullptr;
     bool used = false;
     void _declare(dagc* root);
     int  _assert(dagc* root);

   public:
        Searcher(/* args */);
        ~Searcher();

        void init(Info* i);
        bool search();
        void reset();
        void set_model();
   };
   
} // namespace ismt



#endif