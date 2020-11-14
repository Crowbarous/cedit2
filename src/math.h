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

TEMPLATE_NSQ std::ostream& operator<< (std::ostream& s, const VEC_NSQ& v)
{
	for (int i = 0; i < N-1; i++)
		s << v[i] << ' ';
	return s << v[N-1];
}

TEMPLATE_NSQ std::istream& operator>> (std::istream& s, VEC_NSQ& v)
{
	for (int i = 0; i < N; i++)
		s >> v[i];
	return s;
}

template <typename T> T _read_num (const char*);
#define READ_NUMBER_FLOAT_(T, f) \
	template <> inline T _read_num<T> (const char* s) { return f(s, nullptr); }
#define READ_NUMBER_INT_(T, f) \
	template <> inline T _read_num<T> (const char* s) { return f(s, nullptr, 10); }
READ_NUMBER_FLOAT_(float, strtof);
READ_NUMBER_FLOAT_(double, strtod);
READ_NUMBER_FLOAT_(long double, strtold);
READ_NUMBER_INT_(long, strtol);
READ_NUMBER_INT_(long long, strtoll);
READ_NUMBER_INT_(unsigned long long, strtoull)
#undef READ_NUMBER_FLOAT_
#undef READ_NUMBER_INT_

TEMPLATE_NSQ void atovec (const char* s, VEC_NSQ& v)
{
	for (int i = 0; i < N; i++)
		v[i] = _read_num<S>(s);
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

#endif /* MATH_H */
