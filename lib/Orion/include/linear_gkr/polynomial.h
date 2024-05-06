#ifndef __polynomial
#define __polynomial
/** @brief several univariate polynomial definitions. 
    */
#include <vector>
#include "linear_gkr/prime_field.h"

class linear_poly;
/**Construct a univariate cubic polynomial of f(x) = ax^3 + bx^2 + cx + d
        */
class cubic_poly
{
public:
	prime_field::field_element a, b, c, d;
	cubic_poly();
	cubic_poly(const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&);
	cubic_poly operator + (const cubic_poly &) const;
	prime_field::field_element eval(const prime_field::field_element &) const;
};

/**Construct a univariate quadratic polynomial of f(x) = ax^2 + bx + c
        */
class quadratic_poly
{
public:
	prime_field::field_element a, b, c;
	quadratic_poly();
	quadratic_poly(const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&);
	quadratic_poly operator + (const quadratic_poly &) const;
	cubic_poly operator * (const linear_poly &) const;
	prime_field::field_element eval(const prime_field::field_element &) const;
};

/**Construct a univariate linear polynomial of f(x) = ax + b
        */
class linear_poly
{
public:
	prime_field::field_element a, b;
	linear_poly();
	linear_poly(const prime_field::field_element &, const prime_field::field_element &);
	linear_poly(const prime_field::field_element &);
	linear_poly operator + (const linear_poly &) const;
	quadratic_poly operator * (const linear_poly &) const;
	prime_field::field_element eval(const prime_field::field_element &) const;
};



/**Construct a univariate quintuple polynomial of f(x) = ax^4 + bx^3 + cx^2 + dx + e
        */
class quadruple_poly
{
public:
	prime_field::field_element a, b, c, d, e;
	quadruple_poly();
	quadruple_poly(const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&);
	quadruple_poly operator + (const quadruple_poly &) const;
	prime_field::field_element eval(const prime_field::field_element &) const;
};

/**Construct a univariate quintuple polynomial of f(x) = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
        */
class quintuple_poly
{
public:
	prime_field::field_element a, b, c, d, e, f;
	quintuple_poly();
	quintuple_poly(const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&, const prime_field::field_element&);
	quintuple_poly operator + (const quintuple_poly &) const;
	prime_field::field_element eval(const prime_field::field_element &) const;
};

#endif
