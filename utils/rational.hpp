/* rational.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _RATIONAL_H
#define _RATIONAL_H

namespace ismt
{
    typedef mpz_class Integer;
    class rational
    {
    private:
        /* data */
        Integer numerator(0), 
    public:
        // constructors
        rational():numerator(0), denominator(1) { }
        rational(const integert &i):numerator(i), denominator(1) { }
        rational(int i):numerator(i), denominator(1) { }
        
        integert get_numerator() const
        {
            return numerator;
        }
        
        integert get_denominator() const
        {
            return denominator;
        }
        
        rational &operator+=(const rational &n)
        {
            rational tmp(n);
            same_denominator(tmp);
            numerator+=tmp.numerator;
            normalize();
            return *this;
        }

        rational &operator-=(const rational &n)
        {
            rational tmp(n);
            same_denominator(tmp);
            numerator-=tmp.numerator;
            normalize();
            return *this;
        }

        rational &operator*=(const rational &n)
        {
            numerator*=n.numerator;
            denominator*=n.denominator;
            normalize();
            return *this;
        }

        rational &operator/=(const rational &n)
        {
            assert(!n.numerator.is_zero());
            numerator*=n.denominator;
            denominator*=n.numerator;
            normalize();
            return *this;
        }

        bool operator==(const rational &n) const
        {
            rational r1(*this), r2(n);
            r1.same_denominator(r2);
            return r1.numerator==r2.numerator;
        }
        
        bool operator!=(const rational &n) const
        {
            rational r1(*this), r2(n);
            r1.same_denominator(r2);
            return r1.numerator!=r2.numerator;
        }
        
        bool operator<(const rational &n) const
        {
            rational r1(*this), r2(n);
            r1.same_denominator(r2);
            return r1.numerator<r2.numerator;
        }
        
        bool operator<=(const rational &n) const
        {
            rational r1(*this), r2(n);
            r1.same_denominator(r2);
            return r1.numerator<=r2.numerator;
        }
        
        bool operator>=(const rational &n) const
        {
            return !(*this<n);
        }
        
        bool operator>(const rational &n) const
        {
            return !(*this<=n);
        }
        
        bool is_zero() const
        { return numerator.is_zero(); }
        
        bool is_negative() const
        { return !is_zero() && numerator.is_negative(); }

        void invert();
        void negate() { numerator.negate(); }

        friend rational operator+(const rational &a, const rational &b)
        {
            rational tmp(a);
            tmp+=b;
            return tmp;
        }

        friend rational operator-(const rational &a, const rational &b)
        {
            rational tmp(a);
            tmp-=b;
            return tmp;
        }

        friend rational operator-(const rational &a)
        {
            rational tmp(a);
            tmp.negate();
            return tmp;
        }

        friend rational operator*(const rational &a, const rational &b)
        {
            rational tmp(a);
            tmp*=b;
            return tmp;
        }

        friend rational operator/(const rational &a, const rational &b)
        {
            rational tmp(a);
            tmp/=b;
            return tmp;
        }

        friend std::ostream& operator<< (std::ostream& out, const rational &a);
    };
    
} // namespace ismt


#endif

