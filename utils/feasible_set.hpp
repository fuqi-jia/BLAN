/* feasible_set.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _FEASIBLE_SET_H
#define _FEASIBLE_SET_H

#include "utils/interval.hpp"
#include <cassert>
#include <cstdlib>
#include <list>

const int seed = 2022;

namespace ismt
{
    class feasible_set // a list of disjoint intervals.
    {
    private:
        std::list<Interval> data;
    public:
        feasible_set(){ srand(seed); }
        feasible_set(Interval interval){ srand(seed); data.emplace_back(interval); }
        ~feasible_set(){}
        size_t size() const { return data.size(); }

        void deleteInterval(const Interval& interval){
            // it is in an default order.
            // e.g. (-oo, -12], [-10, -1], [1, 8], [12, +oo)
            if(isSetEmpty()){
                return;
            }
            else if(isFull(interval)){
                // all - (-oo, +oo) = empty
                data.clear();
                return;
            }
            assert(!isFull(interval) && !isSetEmpty());
            std::list<Interval>::iterator it = data.begin();
            // 1. (-oo, 1] - [2, 3] =(-oo, 1]
            // 2.1. (-oo, +oo) - [2, 3] = (-oo, 1], [4, +oo)
            // 2.2. (-oo, 4] - [2, 3] = (-oo, 1], [4, 4]
            // 3. (-oo, 2] - [2, 3] = (-oo, 1]
            while(it!=data.end()){
                if(isFull(*it)){
                    // (-oo, +oo)
                    it = data.erase(it); // delete (-oo, +oo)
                    assert(it==data.end()); // otherwise infinite loop
                    // interval is not full.
                    if(ninf(interval)){
                        // (-oo, +oo) - (-oo, y] = [y+1, +oo)
                        poly::Integer l = getUpper(interval); ++l;
                        data.insert(it, Interval(l, false, Value::plus_infty(), true));
                    }
                    else if(pinf(interval)){
                        // (-oo, +oo) - [x, +oo) = (-oo, x-1]
                        poly::Integer u = getLower(interval); --u;
                        data.insert(it, Interval(Value::minus_infty(), true, u, false));
                    }
                    else{
                        // (-oo, +oo) - [x, y] = (-oo, x-1], [y+1, +oo)
                        poly::Integer l = getUpper(interval); ++l;
                        poly::Integer u = getLower(interval); --u;
                        data.insert(it, Interval(Value::minus_infty(), true, u, false));
                        data.insert(it, Interval(l, false, Value::plus_infty(), true));
                    }
                    return; // done
                }
                else if(ninf(*it)){
                    // (-oo, y]
                    if(ninf(interval)){
                        // (-oo, y] - (-oo, b]
                        int ans = poly::compare_upper(*it, interval);

                        if(ans<=0){
                            // (-oo, 3] - (-oo, 4] = empty
                            it = data.erase(it);
                        }
                        else{ // if(y>b)
                            // (-oo, 5] - (-oo, 4] = [5, 5]
                            poly::Integer l = getUpper(interval); ++l;
                            it->set_lower(l, false);
                            // because interval list obeys the order.
                            break;
                        }
                    }
                    else if(pinf(interval)){
                        // (-oo, y] - [a, +oo)

                        if(poly::get_upper(*it)<poly::get_lower(interval)){
                            // (-oo, 3] - [4, +oo) = (-oo, 3]
                            ++it;
                        }
                        else{ //  if(y>=a)
                            // (-oo, 3] - [2, +oo) = (-oo, 1]
                            poly::Integer u = getLower(interval); --u;
                            it->set_upper(u, false);
                            ++it;
                        }
                    }
                    else{
                        // (-oo, y] - [a, b]

                        if(poly::get_upper(*it)<poly::get_lower(interval)){
                            // (-oo, 3] - [4, 5] = (-oo, 3]
                            ++it;
                        }
                        else if(poly::get_lower(interval) <= poly::get_upper(*it) && poly::get_upper(*it) <= poly::get_upper(interval)){
                            // (-oo, 3] - [2, 5] = (-oo, 1]
                            poly::Integer u = getLower(interval); --u;
                            it->set_upper(u, false);
                            ++it;
                        }
                        else{ // if(y>b)
                            // (-oo, 3] - [1, 2] = (-oo, 0], [3, 3]
                            poly::Integer a = getLower(interval); --a;
                            poly::Integer b = getUpper(interval); ++b;
                            poly::Integer ou = getUpper(*it);
                            it = data.erase(it);
                            data.insert(it, Interval(Value::minus_infty(), true, a, false));
                            data.insert(it, Interval(b, false, ou, false));
                            // because interval list obeys the order.
                            break;
                        }
                    }
                }
                else if(pinf(*it)){
                    // [x, +oo)
                    if(ninf(interval)){
                        // [x, +oo) - (-oo, b]

                        if(poly::get_lower(*it)<=poly::get_upper(interval)){
                            // [3, +oo) - (-oo, 4] = [5, +oo)
                            poly::Integer b = getUpper(interval); ++b;
                            it->set_lower(b, false);
                        }
                        else{ // if(x>b)
                            // [3, +oo) - (-oo, 2] = [3, +oo)
                        }
                    }
                    else if(pinf(interval)){
                        // [x, +oo) - [a, +oo)
                        int ans = poly::compare_lower(*it, interval);

                        if(ans<0){
                            // [3, +oo) - [4, +oo) = [3, 3]
                            poly::Integer a = getLower(interval); --a;
                            it->set_upper(a, false);
                        }
                        else{ // if(x>=a)
                            // [3, +oo) - [2, +oo) = empty
                            it = data.erase(it);
                        }
                    }
                    else{
                        // [x, +oo) - [a, b]

                        if(poly::get_lower(*it)>poly::get_upper(interval)){
                            // [3, +oo) - [1, 2] = [3, +oo)
                        }
                        else if(poly::get_lower(interval) <= poly::get_lower(*it) && poly::get_lower(*it) <= poly::get_upper(interval)){
                            // [3, +oo) - [2, 5] = [6, +oo)
                            poly::Integer b = getUpper(interval); ++b;
                            it->set_lower(b, false);
                        }
                        else{ // if(x<a)
                            // [3, +oo) - [4, 5] = [3, 3], [6, +oo)
                            poly::Integer a = getLower(interval); --a;
                            poly::Integer b = getUpper(interval); ++b;
                            poly::Integer ol = getLower(*it);
                            it = data.erase(it);
                            data.insert(it, Interval(ol, false, a, false));
                            data.insert(it, Interval(b, false, Value::plus_infty(), true));
                        }
                    }
                    // because interval list obeys the order.
                    // only one interval with plus infinity.
                    break;
                }
                else{
                    // [x, y] 

                    if(ninf(interval)){
                        // [x, y] - (-oo, b]
                        if(poly::get_lower(*it) > poly::get_upper(interval)){
                            // [3, 6] - (-oo, 2] = [3, 6]
                            // because interval list obeys the order.
                            break;
                        }
                        else if(poly::get_upper(*it) > poly::get_upper(interval)){ // x<=b
                            // [3, 6] - (-oo, 4] = [5, 6]
                            poly::Integer b = getUpper(interval); ++b;
                            it->set_lower(b, false);
                            // because interval list obeys the order.
                            break;
                        }
                        else{ // x<=b && y<=b
                            // [3, 6] - (-oo, 6] = empty
                            it = data.erase(it);
                        }
                    }
                    else if(pinf(interval)){
                        // [x, y] - [a, +oo)
                        if(poly::get_upper(*it) < poly::get_lower(interval)){
                            // [3, 6] - [7, +oo) = [3, 6]
                            ++it;
                        }
                        else if(poly::get_lower(*it) < poly::get_lower(interval)){
                            // [3, 6] - [4, +oo) = [3,3]
                            poly::Integer a = getLower(interval); --a;
                            it->set_upper(a, false);
                            ++it;
                        }
                        else{
                            // [3, 6] - [3, +oo) = empty
                            it = data.erase(it);
                        }
                    }
                    else{
                        // [x, y] - [a, b]
                        poly::Integer a = getLower(interval);
                        poly::Integer b = getUpper(interval);
                        poly::Integer x = getLower(*it);
                        poly::Integer y = getUpper(*it);

                        if(y<a){
                            // [3, 6] - [7, 8] = [3, 6]
                            ++it;
                        }
                        else if(x<a && y<=b){
                            // [3, 6] - [4, 6] = [3,3]
                            --a;
                            it->set_upper(a, false);
                            ++it;
                        }
                        else if(x>=a && y<=b){
                            // [3, 6] - [2, 7] = empty
                            it = data.erase(it);
                        }
                        else if(x<a && y>b){
                            // [3, 6] - [4, 5] = [3, 3], [6, 6]
                            it = data.erase(it);
                            --a; ++b;
                            data.insert(it, Interval(x, false, a, false));
                            data.insert(it, Interval(b, false, y, false));
                            return; // done
                        }
                        else if(x<=b && y>b){
                            // [3, 6] - [3, 5] = [6, 6]
                            ++b;
                            it->set_lower(b, false);
                            ++it;
                        }
                        else{ // x > b
                            // [3, 6] - [1, 2] = [3, 6]
                            ++it;
                        }
                    }
                }
            }
        }

        void addInterval(const Interval& interval){
            // union interval
            // it is in an default order. 
            // e.g. (-oo, -12], [-10, -1], [1, 8], [12, +oo)
            // interval must be vaild.
            if(isSetFull()){
                return;
            }
            else if(isFull(interval)){
                // all + (-oo, +oo) = (-oo, +oo)
                data.clear();
                data.emplace_back(FullInterval);
                return;
            }
            else if(data.size() == 0){
                data.insert(data.begin(), interval);
                return;
            }

            assert(!isSetFull() && !isFull(interval));

            // insert into data
            std::list<Interval>::iterator it = data.begin();
            // 1. (-oo, 1] + [2, 3] =(-oo, 1]
            // 2. (-oo, 2] + [1, 3] = (-oo, 3]
            if(ninf(interval)){
                // compare with header: (-oo, y] + (-oo, b]
                Interval header = data.front();
                int ans = poly::compare_upper(data.front(), interval);

                if(ans <= 0){
                    // (-oo, 3] + (-oo, 4] = (-oo, 4]
                    poly::Integer b = getUpper(interval);
                    data.front().set_upper(b, false);
                }
                else{ // if(y>=b)
                    // (-oo, 5] + (-oo, 4] = (-oo, 5] -> done
                }
                // (-oo, 3] + [4, 4] = (-oo, 4] // new 05/29
                if(!ninf(data.front())) data.front().set_lower(poly::Value::minus_infty(), true);
            }
            else if(pinf(interval)){
                // compare with tail: [x, +oo) + [a, +oo)

                int ans = poly::compare_lower(data.back(), interval);

                if(ans < 0){
                    // [3, +oo) + [4, +oo) = [3, +oo) -> done
                }
                else{ // if(x>a)
                    // [3, +oo) + [2, +oo) = [2, +oo)
                    poly::Integer a = getLower(interval);
                    data.back().set_lower(a, false);
                }
                // [3, +oo) + [2, 2] = [2, +oo) // new 05/29
                if(!pinf(data.back())) data.back().set_upper(poly::Value::plus_infty(), true);
            }
            else{
                bool inserted = false;
                while(it!=data.end()){
                    if(ninf(*it)){
                        // (-oo, y] + [a, b]
                        poly::Integer y = getUpper(*it);
                        poly::Integer a = getLower(interval); --a;
                        poly::Integer b = getUpper(interval);
                        if(y <= a){
                            // (-oo, 4] + [3, 6] = (-oo, 6]
                            // (-oo, 2] + [3, 6] = (-oo, 6]
                            // insert
                            ++it;
                            data.insert(it, interval);
                            inserted = true;
                            break;
                        }
                    }
                    else if(pinf(*it)){
                        // [x, +oo) + [a, b]
                        data.insert(it, interval);
                        inserted = true;
                        break;
                    }
                    else{
                        // [x, y] + [a, b]
                        poly::Integer x = getLower(*it);
                        poly::Integer y = getUpper(*it);
                        poly::Integer b = getUpper(interval);
                        poly::Integer a = getLower(interval);
                        if(overlap(*it, interval)){
                            it->set_lower((x>a?a:x), false);
                            it->set_upper((y>b?y:b), false);
                            inserted = true;
                            break;
                        }
                        else{
                            --x;
                            if(b <= x){
                                // [2, 3] [0, 1]
                                // insert [4] into { [8] }: b = 4, x = 7
                                data.insert(it, interval);
                                inserted = true;
                                break;
                            }
                        }
                    }
                    ++it;
                }
                if(!inserted) data.emplace_back(interval);
            }
            // compress the overlap
            compress();
        }
        void compress(){
            // compress the list, once objects overlap
            std::list<Interval>::iterator it = data.begin();
            // Greedily combining
            while(true){
                if(data.size()==1) return; // nothing to do.
                auto cur = it;
                ++it;
                if(it == data.end() || cur == data.end()) break; // 05/30
                auto pit = it;
                // only need to compare with the next element.
                // for add interval and delete interval, maintain partial order;
                // partial order is [x, y] <= [a, b]
                // if x = -oo and y != +oo, then b >= y;
                // if x != -oo and y != +oo, then a >= x and b >= y;
                // if x = -oo and y = +oo, then a = -oo, b = +oo;
                // if x != -oo and y = +oo, then a >= x.
                // for example, and at add interval, 
                //      if data[i-1] is [x, +oo), data[i] is [a(>x), +oo).
                if(isFull(*cur)){
                    // (-oo, +oo)
                    data.clear();
                    data.emplace_back(*cur);
                    return;
                }
                else if(ninf(*cur)){
                    // (-oo, f]
                    // f is not +/-oo.
                    // assumes current element [x, y], the next element [a, b].
                    // after add interval [c, d], classified discussion, 
                    // 1. add (-oo, +oo), return.
                    // 2. add (-oo, c], c < x-1: 
                    //      2.1. [a, b] has: a >= x, so a is not +/-oo.
                    // 3. add (-oo, c], c >= x-1: 
                    //      3.1. a - 1 <= c, then [a, b] -> (-oo, f]
                    //      3.2. a - 1 > c, then [a, b] -> [a, b]
                    // 4. add [c, +oo), c > y+1: 
                    //      3.1. b + 1 >= c, then [a, b] -> [e, +oo)
                    //      3.2. b + 1 >= c, then [a, b] -> [e, +oo)
                    // 5. add [c, d], then [a, b] both are not +/-oo.
                    // In summary, [a, b] -> [e, f] or [e, +oo) or (-oo, f]

                    // try to expand its upper bound.
                    while(pit!=data.end()){
                        // eat all (-oo, b] and [a(<=y+1), b]
                        if(isFull(*pit)){
                            data.clear();
                            data.emplace_back(FullInterval);
                            return;
                        }
                        else if(ninf(*pit)){
                            // pit is (-oo, b]
                            poly::Integer y = getUpper(*pit);
                            poly::Integer b = getUpper(*cur);
                            cur->set_upper((y<b?y:b), false);
                            pit = data.erase(pit);
                        }
                        else{
                            // [a, b] or [a, +oo)
                            poly::Integer r = getUpper(*cur); ++r;
                            if(getLower(*pit)<=r){
                                if(pinf(*pit)){
                                    // (-oo, 3], [4, +oo) -> (-oo, +oo)
                                    data.clear();
                                    data.emplace_back(FullInterval);
                                    return;
                                }
                                else{
                                    // (-oo, 3], [4, 5] -> (-oo, 5]
                                    poly::Integer y = getUpper(*pit);
                                    poly::Integer b = getUpper(*cur);
                                    cur->set_upper((y<b?y:b), false);
                                    pit = data.erase(pit);
                                }
                            }
                            else break; // because of in order
                        }
                    }
                }
                else if(pinf(*cur)){
                    // [e, +oo)
                    // e is not +/-oo.
                    // assumes current element [x, y], the next element [a, b].
                    // after add interval [c, d], classified discussion, 
                    // 1. add (-oo, +oo), return.
                    // 2. add (-oo, c], then c < x-1: 
                    //      2.1. [a, b] has: a >= x, so a is not -oo.
                    // 3. add [c, +oo), c > y+1: 
                    //      3.1. b + 1 >= c, then [a, b] -> [e, +oo)
                    //      3.2. b + 1 < c, then [a, b] -> [a, b] 
                    // 4. add [c, +oo), c <= y+1: 
                    //      3.1. b + 1 >= c, then [a, b] -> [e, +oo)
                    //      3.2. b + 1 < c, then [a, b] -> [a, b] 
                    // 4. add [c, d], then [a, b] both are not +/-oo.
                    // In summary, [a, b] -> [e, f] or [e, +oo).

                    // try to expand its lower bound.
                    while(pit!=data.end()){
                        // eat all [a, +oo) and [a, b(>=x-1)]
                        if(isFull(*pit)){
                            data.clear();
                            data.emplace_back(FullInterval);
                            return;
                        }
                        else if(pinf(*pit)){
                            // pit is [e, +oo)
                            poly::Integer x = getLower(*pit);
                            poly::Integer a = getLower(*cur);
                            cur->set_lower((x>a?a:x), false);
                            pit = data.erase(pit);
                        }
                        else{
                            // [a, b] or (-oo, b]
                            poly::Integer r = getUpper(*cur); --r;
                            if(getUpper(*pit)>=r){
                                // means that [e, +oo) -> (-oo, +oo). NEVER.
                                if(ninf(*pit)){ assert(false); }
                                else{
                                    // [3, +oo), [1, 2] -> [1, +oo)
                                    poly::Integer x = getLower(*pit);
                                    poly::Integer a = getLower(*cur);
                                    cur->set_lower((x>a?a:x), false);
                                    pit = data.erase(pit);
                                }
                            }
                            else break; // because of in order
                        }
                    }
                }
                else{
                    // [x, y] both are not +/-oo.
                    // assumes current element [x, y], the next element [a, b].
                    // after add interval [c, d], classified discussion, 
                    // 1. add (-oo, +oo), return.
                    // 2. add (-oo, c], c < x-1: 
                    //      2.1. [a, b] has: a >= x, so [a, b] both are not +/-oo.
                    // 3. add [c, +oo), c > y+1: 
                    //      3.1. b + 1 >= c, then [a, b] -> [a, +oo)
                    //      3.2. b + 1 < c, then [a, b] -> [a, b] 
                    // 4. add [c, d], then [a, b] both are not +/-oo.
                    // In summary, [a, b] -> [e, f] or [a, +oo).

                    // try to expand upper bound.
                    while(pit!=data.end()){
                        // eat all (-oo, b(>=x-1)], [a(<=y+1), +oo) and [a, b] overlaped with cur.
                        if(isFull(*pit)){
                            data.clear();
                            data.emplace_back(FullInterval);
                            return;
                        }
                        else if(ninf(*pit)){
                            assert(false);
                        }
                        else if(pinf(*pit)){
                            poly::Integer r = getUpper(*cur); ++r;
                            if(getLower(*pit)<=r){
                                poly::Integer x = getLower(*pit);
                                poly::Integer a = getLower(*cur);
                                cur->set_lower((x>a?a:x), false);
                                cur->set_upper(Value::plus_infty(), true);
                                // delete all, for we have [a, +oo)
                                pit = data.erase(pit);
                            }
                            else break; // because of in order
                        }
                        else{
                            // [e, f]
                            poly::Integer x = getLower(*pit);
                            poly::Integer y = getUpper(*pit);
                            poly::Integer b = getUpper(*cur);
                            poly::Integer a = getLower(*cur);
                            if(overlap(*pit, *cur)){
                                cur->set_lower((x>a?a:x), false);
                                cur->set_upper((y>b?y:b), false);
                                pit = data.erase(pit);
                            }
                            else{ // 05/30
                                ++y;
                                --x;
                                if(y == a){
                                    // [x, y] + [y+1, b]
                                    --y;
                                    cur->set_lower((x>a?a:x), false);
                                    cur->set_upper((y>b?y:b), false);
                                    pit = data.erase(pit);
                                }
                                else if(x == b){
                                    // [a, x-1] [x, y] but violate the order
                                    ++x;
                                    cur->set_lower((x>a?a:x), false);
                                    cur->set_upper((y>b?y:b), false);
                                    pit = data.erase(pit);
                                }
                                else{
                                    ++pit;
                                }
                            }
                        }
                    }
                }
                it = cur;
                ++it;
            }
        }

        void intersectInterval(const Interval& interval){
            if(isFull(interval)) return; // never empty
            std::list<Interval>::iterator it = data.begin();
            while(it!=data.end()){
                if(overlap(*it, interval)){
                    if(ninf(*it)){
                        poly::Integer y = getUpper(*it);
                        if(ninf(interval)){
                            poly::Integer b = getUpper(interval);
                            it->set_upper((y>b?b:y), false);
                        }
                        else if(pinf(interval)){
                            poly::Integer a = getLower(interval);
                            it->set_lower(a, false);
                            it->set_upper(y, false);
                        }
                        else{
                            poly::Integer b = getUpper(interval);
                            poly::Integer a = getLower(interval);
                            it->set_lower(a, false);
                            it->set_upper((y>b?b:y), false);
                        }
                    }
                    else if(pinf(*it)){
                        poly::Integer x = getLower(*it);

                        if(ninf(interval)){
                            poly::Integer b = getUpper(interval);
                            it->set_upper(b, false);
                        }
                        else if(pinf(interval)){
                            poly::Integer a = getLower(interval);
                            it->set_lower((x>a?x:a), false);
                        }
                        else{
                            poly::Integer b = getUpper(interval);
                            poly::Integer a = getLower(interval);
                            it->set_lower((x>a?x:a), false);
                            it->set_upper(b, false);
                        }
                    }
                    else{
                        // [x, y]
                        poly::Integer x = getLower(*it);
                        poly::Integer y = getUpper(*it);
                        if(ninf(interval)){
                            poly::Integer b = getUpper(interval);
                            it->set_upper((y>b?b:y), false);
                        }
                        else if(pinf(interval)){
                            poly::Integer a = getLower(interval);
                            it->set_lower((x>a?x:a), false);
                        }
                        else{
                            poly::Integer b = getUpper(interval);
                            poly::Integer a = getLower(interval);
                            poly::Integer m = (x>a?x:a);
                            poly::Integer n = (y>b?b:y);
                            if(m>n){it = data.erase(it); continue;}
                            it->set_lower(m, false);
                            it->set_upper(n, false);
                        }
                    }
                    ++it;
                }
                else it = data.erase(it);
            }
            compress();
        }

        void deleteFeasibleSet(feasible_set& fs){
            std::list<Interval>::iterator it = fs.data.begin();
            while(it!=fs.data.end()){
                deleteInterval(*it);
                ++it;
            }
        }

        // add a feasible set to this.
        void addFeasibleSet(feasible_set& fs){
            std::list<Interval>::iterator it = fs.data.begin();
            while(it!=fs.data.end()){
                addInterval(*it);
                ++it;
            }
        }

        void intersectFeasibleSet(feasible_set& fs){
            std::list<Interval>::iterator it = fs.data.begin();
            while(it!=fs.data.end()){
                intersectInterval(*it);
                ++it;
            }
        }

        // set the feasible set as a interval.
        bool setInterval(const Interval& interval){
            data.clear();
            data.emplace_back(interval);
            return true;
        }

        Interval choose(){
            if(data.size()==0) return EmptyInterval;
            else{
                // randomly return an interval
                int s = rand() % data.size();
                auto it = data.begin();
                while(it!=data.end() && (s--)){
                    ++it;
                }
                return (*it);
            }
            // else return *(data.begin()); // return first interval.
        }

        Interval& back(){
            return data.back();
        }
        Interval& front(){
            return data.front();
        }

        void get_intervals(std::vector<Interval>& itvs){
            auto it = data.begin();
            while(it != data.end()){
                itvs.emplace_back(*it);
                ++it;
            }
        }

        Interval get_cover_interval(){
            if(data.size()==0) return FullInterval;
            else{
                Interval be = data.front();
                Interval en = data.back();

                if(ninf(be) && pinf(en)){
                    return FullInterval;
                }
                else if(ninf(be)){
                    return Interval(poly::Value::minus_infty(), true, getUpper(en), poly::get_upper_open(en));
                }
                else if(pinf(en)){
                    return Interval(getLower(be), poly::get_lower_open(be), poly::Value::plus_infty(), true);
                }
                else{
                    return Interval(getLower(be), poly::get_lower_open(be), getUpper(en), poly::get_upper_open(en));
                }
            }
        }

        // checker
        // data.size() == 0 
        // data.size() == 1 and is an empty set, this is done by setInterval.
        bool isPoint(){
            if(data.size() == 1){
                if(poly::is_point(*(data.begin()))) return true;
                else return false;
            }
            else return false;
        }
        Integer getPoint(){
            assert(isPoint());
            return to_long(getRealLower(*(data.begin())));
        }
        bool isSetEmpty(){ return data.size()==0; }
        bool isSetFull(){ 
            if(isFull(*data.begin())){
                assert(data.size()==1);
                return true;
            }
            else return false;
        }

        Integer pick_value(){
            // pick a value
            return to_long(poly::pick_value(*(data.begin())));
        }

        
        friend std::ostream& operator<< (std::ostream& out, const feasible_set &f){
            auto it = f.data.begin();
            out<<"{ "<<*(it);
            ++it;
            while(it!=f.data.end()){
                out<<", "<<*it;
                ++it;
            }
            out<<" }";
            return out;
        }
    };
    
} // namespace ismt



#endif