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
