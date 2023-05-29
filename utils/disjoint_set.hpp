/* disjoint_set.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _DISJOINT_SET_H
#define _DISJOINT_SET_H

#include <vector>
#include <boost/unordered_map.hpp>

namespace ismt
{
    template<class T>
    class disjoint_set
    {
    private:
        T _st;
        T _ed;
        boost::unordered_map<T, T> data;
        // boost::unordered_map<T, int> ranks;
    public:
        disjoint_set(){}
        ~disjoint_set(){}

        void add(const T& x){
            if(data.find(x)!=data.end()){
                return;
            }
            else{
                data[x] = x;
                // ranks[x] = 0;
            }
        }

        T find(T x){
            if(data.find(x)==data.end()){ add(x); return x; }
            if(x == data[x]){
                return x;
            }
            else{
                data[x] = find(data[x]);
                return data[x];
            }
            // return x == data[x]?x:data[x]=find(data[x]);
        }

        void union_set(T x, T y){
            T rootx = find(x);
            T rooty = find(y);
            if(rootx!=rooty){
                data[rootx] = rooty; // never changed the root node.
            }
        }

        bool Equal(T x, T y){
            return find(x)==find(y);
        }

        void clear(){
            data.clear();
        }
    };
    
} // namespace ismt

#endif