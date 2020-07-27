#ifndef MATH_H
#define MATH_H

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

#define TEMPLATE_NSQ template<int N, class S = float, glm::qualifier Q = glm::packed>
#define VEC_NSQ glm::vec<N, S, Q>

TEMPLATE_NSQ std::ostream& operator<< (std::ostream& s, VEC_NSQ& v)
{
	for (int i = 0; i < N-1; i++)
		s << v[i] << ' ';
	return s << v[N-1];
}

TEMPLATE_NSQ std::istream& operator>> (std::istream& s, const VEC_NSQ& v)
{
	for (int i = 0; i < N; i++)
		s >> v[i];
	return s;
}

/*
 * Can't use std::from_chars() since that wants to know where the buffer ends.
 * Also, strtof and friends for some reason want a char** and not const char**
 * for the second argument, so we const_cast once later
 */
template <class T> inline auto _read_number (char*);
template <> inline auto _read_number<float> (char* s) { return strtof(s, &s); }
template <> inline auto _read_number<double> (char* s) { return strtod(s, &s); }
template <> inline auto _read_number<long double> (char* s) { return strtold(s, &s); }

TEMPLATE_NSQ void atovec (const char* s, VEC_NSQ& v)
{
	for (int i = 0; i < N; i++)
		v[i] = _read_number<S>(const_cast<char**>(s));
}
TEMPLATE_NSQ void atovec (const std::string& s, VEC_NSQ& v)
{
	atovec(s.c_str(), v);
}
TEMPLATE_NSQ VEC_NSQ atovec (const char* s)
{
	VEC_NSQ v;
	atovec(s, v);
	return v;
}
TEMPLATE_NSQ VEC_NSQ atovec (const std::string& s)
{
	VEC_NSQ v;
	atovec(s, v);
	return v;
}

#undef TEMPLATE_NSQ
#undef VEC_NSQ

#endif // MATH_H
