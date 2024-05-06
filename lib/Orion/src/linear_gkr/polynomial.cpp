#include "linear_gkr/polynomial.h"

quintuple_poly::quintuple_poly(){}
quintuple_poly::quintuple_poly(const prime_field::field_element& aa, const prime_field::field_element& bb, const prime_field::field_element& cc, const prime_field::field_element& dd, const prime_field::field_element& ee, const prime_field::field_element& ff)
{
	a = aa;
	b = bb;
	c = cc;
	d = dd;
	e = ee;
	f = ff;
}

quintuple_poly quintuple_poly::operator + (const quintuple_poly &x) const
{
	return quintuple_poly(a + x.a, b + x.b, c + x.c, d + x.d, e + x.e, f + x.f);
}

prime_field::field_element quintuple_poly::eval(const prime_field::field_element &x) const
{
	return (((((a * x) + b) * x + c) * x + d) * x + e) * x + f;
}

quadruple_poly::quadruple_poly(){}
quadruple_poly::quadruple_poly(const prime_field::field_element& aa, const prime_field::field_element& bb, const prime_field::field_element& cc, const prime_field::field_element& dd, const prime_field::field_element& ee)
{
	a = aa;
	b = bb;
	c = cc;
	d = dd;
	e = ee;
}

quadruple_poly quadruple_poly::operator + (const quadruple_poly &x) const
{
	return quadruple_poly(a + x.a, b + x.b, c + x.c, d + x.d, e + x.e);
}

prime_field::field_element quadruple_poly::eval(const prime_field::field_element &x) const
{
	return ((((a * x) + b) * x + c) * x + d) * x + e;
}

cubic_poly::cubic_poly(){}
cubic_poly::cubic_poly(const prime_field::field_element& aa, const prime_field::field_element& bb, const prime_field::field_element& cc, const prime_field::field_element& dd)
{
	a = aa;
	b = bb;
	c = cc;
	d = dd;
}

cubic_poly cubic_poly::operator + (const cubic_poly &x) const
{
	return cubic_poly(a + x.a, b + x.b, c + x.c, d + x.d);
}

prime_field::field_element cubic_poly::eval(const prime_field::field_element &x) const
{
	return (((a * x) + b) * x + c) * x + d;
}


quadratic_poly::quadratic_poly(){}
quadratic_poly::quadratic_poly(const prime_field::field_element& aa, const prime_field::field_element& bb, const prime_field::field_element& cc)
{
	a = aa;
	b = bb;
	c = cc;
}

quadratic_poly quadratic_poly::operator + (const quadratic_poly &x) const
{
	return quadratic_poly(a + x.a, b + x.b, c + x.c);
}

cubic_poly quadratic_poly::operator * (const linear_poly &x) const
{
	return cubic_poly(a * x.a, a * x.b + b * x.a, b * x.b + c * x.a, c * x.b);
}

prime_field::field_element quadratic_poly::eval(const prime_field::field_element &x) const
{
	return ((a * x) + b) * x + c;
}





linear_poly::linear_poly(){}
linear_poly::linear_poly(const prime_field::field_element& aa, const prime_field::field_element& bb)
{
	a = aa;
	b = bb;
}
linear_poly::linear_poly(const prime_field::field_element &x)
{
	a = prime_field::field_element(0);
	b = x;
}

linear_poly linear_poly::operator + (const linear_poly &x) const
{
	return linear_poly(a + x.a, b + x.b);
}

quadratic_poly linear_poly::operator * (const linear_poly &x) const
{
	return quadratic_poly(a * x.a, a * x.b + b * x.a, b * x.b);
}

prime_field::field_element linear_poly::eval(const prime_field::field_element &x) const
{
	return a * x + b;
}
