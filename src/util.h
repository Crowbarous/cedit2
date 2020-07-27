#ifndef UTIL_H
#define UTIL_H

#include <cstdint>

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

uint64_t ceil_po2 (uint64_t);
uint32_t ceil_po2 (int32_t);

#endif // UTIL_H

