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

template <class T> constexpr bool always_false = false;

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
	else
		static_assert(always_false<T>, "Called ceil_po2 on an invalid type");
}

template <class T>
std::ostream& debug_print_container (const T& v, std::ostream& s)
{
	s << "[ ";
	for (const auto& elem: v)
		s << elem << ' ';
	return s << ']';
}

bool str_any_of (const char* needle,
		std::initializer_list<const char*> haystack);

/*
 * Wrap a fundamental type into a class. Two different wrappers to the
 * same type are prevented from *implicitly* being cast into eath other
 *
 * (for example, indices into unrelated data structures may both be
 *  integer but it often makes no sense to cast them into each other)
 *
 * Note: anything that isn't simple assignment goes through anyway:
 *       arithmetics on different wrappers, etc.
 */
#define WRAP_FUNDAMENTAL_TYPE(fundamental_type, class_name) \
	class class_name { \
	private: \
		fundamental_type value; \
	public: \
		class_name (const fundamental_type& v): value(v) { } \
		operator fundamental_type () { return value; } \
	}

#endif /* UTIL_H */

