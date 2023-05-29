/* simp_vector.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _SIMP_VECTOR_H
#define _SIMP_VECTOR_H

#include <vector>

namespace ismt
{   
    // a vector that support minus index
    // once it is emplace_back zero_index ++
    // a[-1] = a.data[zero_index-1]
    // a[1]  = a.data[zero_index+1]
    template<typename T>
    class simp_vector
    {
    private:
        T zero;
        std::vector<T> positive; // +1 -> +oo
        std::vector<T> negative; // -1 -> -oo
        size_t size = 0;
    public:
        simp_vector(){}
        simp_vector(size_t size){ 
            positive.resize(size);
            negative.resize(size);
        }
        ~simp_vector(){}

        size_t size() const { return size; }
        void emplace_back(const T& t){
            if(size == 0){
                zero = t; ++size; return;
            }
            // insert front
            positive.emplace_back(t);
            // insert back
            negative.emplace_back(t);
            // duplicate the item
            size += 2;
        }

        T& operator[](int index){
            assert(index >= -negative.size());
            assert(index <= positive.size());
            if(index > 0) return data[index-1];
            else if(index < 0) return data[1-index];
            else return zero;
        }

        T& operator[](int index) const {
            assert(index >= -negative.size());
            assert(index <= positive.size());
            if(index > 0) return data[index+1];
            else if(index < 0) return data[1-index];
            else zero;
        }
    };
    
} // namespace ismt
