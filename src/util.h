#ifndef UTIL_H
#define UTIL_H

#include <iostream>

#define DEBUG_EXPR(expr) \
	do { \
		std::cerr << #expr " = " << (expr) << std::endl; \
	} while (false)

#define DEBUG_MSG(expr) \
	do { \
		std::cerr << "d: " << (expr) << std::endl; \
	} while (false)

#define likely(x) __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false)

void fatal [[noreturn]] (const char* fmt, ...);
void warning (const char* fmt, ...);

template <class T>
T ceil_po2 (T x)
{
	if (x == 1)
		return 1;
	if constexpr (sizeof(T) == 4)
		return T{1} << (32 - __builtin_clz(x - 1));
	else if constexpr (sizeof(T) == 8)
		return T{1} << (64 - __builtin_clzl(x - 1));
	else
		static_assert(!sizeof(T), "ceil_po2() argument must be 32 or 64 bit int");
}


template <class T>
std::ostream& debug_print_container (const T& v, std::ostream& s)
{
	s << "[ ";
	for (const auto& elem: v)
		s << elem << ' ';
	return s << ']';
}

template <class T>
void replace_with_last (T& container, int index)
{
	if (container.size() > 1)
		container[index] = container.back();
	container.pop_back();
}


bool str_any_of (const char* needle, std::initializer_list<const char*> haystack);

#endif /* UTIL_H */

