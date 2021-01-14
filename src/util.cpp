#include "util.h"
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>

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

void warning (const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "Warning: ");
	if (fmt)
		vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);
}

bool str_any_of (const char* needle,
		std::initializer_list<const char*> haystack)
{
	for (const char* s: haystack) {
		if (strcmp(needle, s) == 0)
			return true;
	}
	return false;
}
