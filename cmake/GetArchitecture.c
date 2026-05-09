#include <stdio.h>

int main()
{
#if defined(_M_X64) || defined(__amd64)
	puts("x86_64");
#elif defined(_M_IX86) || defined(__i386__)
	puts("x86");
#elif defined(_M_ARM64) || defined(__aarch64__)
	puts("aarch64");
#else
	return 1;
#endif
	return 0;
}
