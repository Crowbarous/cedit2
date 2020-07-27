#include "util.h"
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

[[noreturn]] void fatal (const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "Fatal: ");
	if (fmt)
		vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);

	exit(EXIT_FAILURE);
}

int warning (const char* fmt, ...)
{
	int result = 0;

	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "Warning: ");
	if (fmt)
		result = vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);

	return result;
}

uint64_t ceil_po2 (uint64_t x)
{
	if (x == 1)
		return 1;
	return 1 << (64 - __builtin_clzl(x-1));
}

uint32_t ceil_po2 (uint32_t x)
{
	if (x == 1)
		return 1;
	return 1 << (32 - __builtin_clz(x-1));
}
