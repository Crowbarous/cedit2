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

[[noreturn]] void fatal (const char* fmt, ...);
int warning (const char* fmt, ...);

template <class T> T ceil_po2 (T x)
{
	if (x == 1)
		return 1;

	constexpr bool _32 = (sizeof(T) == 4);
	constexpr bool _64 = (sizeof(T) == 8);
	static_assert(std::is_integral_v<T> && (_32 || _64),
			"ceil_po2() argument must be 32- or 64-bit int");

	if constexpr (_32)
		return 1 << (32 - __builtin_clz(x-1));
	else if constexpr (_64)
		return 1 << (64 - __builtin_clzl(x-1));
}

template <class T>
std::ostream& debug_print_container (const T& v, std::ostream& s)
{
	s << "[ ";
	for (const auto& elem: v)
		s << elem << ' ';
	return s << ']';
}

#endif // UTIL_H

