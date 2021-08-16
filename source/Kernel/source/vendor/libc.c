
#include <stdint.h>
#include <stdlib.h>

extern void putc(char);
void _putchar(char c)
{
#ifdef _WIN32
	putc(c);
#endif
}


#ifdef __cplusplus
extern "C" {
#endif

void __cxa_pure_virtual()
{
	// TODO
}

// TODO
uint64_t mach_absolute_time()
{
	return 0;
}
#if 1
#ifndef _WIN32
void* memcpy(void* dest, const void* src, size_t len)
{
	if (len % 4 == 0) {
		unsigned* udest = (unsigned*)dest;
		unsigned* usrc = (unsigned*)src;
		for (unsigned i = 0; i < len / 4; ++i) {
			udest[i] = usrc[i];
		}
		return dest;
	}
	char* d = (char*)dest;
	const char* s = (const char*)src;
	while (len--)
		*d++ = *s++;
	return dest;
}
void* memset(void* dest, int val, size_t len)
{
	if (len % 8 == 0 && ((unsigned)dest) % 8 == 0) {
		unsigned dlen = len / 8;
		volatile double* ddest = (volatile double*)dest;
		union {
			unsigned uval[2];
			double dval;
		} u;
		u.uval[0] = val;
		u.uval[1] = val;
		double dval = u.dval;
		unsigned dd = dlen - dlen % 8;
		for (unsigned i = 0; i < dd; i += 8) {
			ddest[i] = dval;
			ddest[i+1] = dval;
			ddest[i + 2] = dval;
			ddest[i + 3] = dval;
			ddest[i + 4] = dval;
			ddest[i + 5] = dval;
			ddest[i + 6] = dval;
			ddest[i + 7] = dval;
		}

		unsigned rem = dlen % 8;
		for (unsigned i = 0; i < rem; ++i) {
			ddest[i] = dval;
		}
		return dest;
	}
	if (len % 4 == 0) {
		unsigned ulen = len / 4;
		unsigned* udest = (unsigned*)dest;
		for (unsigned i = 0; i < ulen; ++i) {
			udest[i] = val;
		}
		return dest;
	}

	unsigned char* ptr = (unsigned char*)dest;
	while (len-- > 0)
		*ptr++ = val;
	return dest;
}

void*
memmove(void* dest, const void* src, size_t len)
{
	char* d = (char*)dest;
	const char* s = (const char*)src;
	if (d < s)
		while (len--)
			*d++ = *s++;
	else
	{
		char* lasts = (char*)s + (len - 1);
		char* lastd = d + (len - 1);
		while (len--)
			*lastd-- = *lasts--;
	}
	return dest;
}

size_t strlen(const char* str)
{
	const char* s;

	for (s = str; *s; ++s);
	return(s - str);
}

int memcmp(const void* str1, const void* str2, size_t count)
{
	const unsigned char* s1 = (const unsigned char*)str1;
	const unsigned char* s2 = (const unsigned char*)str2;

	while (count-- > 0)
	{
		if (*s1++ != *s2++)
			return s1[-1] < s2[-1] ? -1 : 1;
	}
	return 0;
}

int strcmp(const char* s1, const char* s2)
{
	int r = -1;

	if (s1 == s2)
	{
		// short circuit - same string
		return 0;
	}

	// I don't want to panic with a NULL ptr - we'll fall through and fail w/ -1
	if (s1 && s2)
	{
		// iterate through strings until they don't match or s1 ends (null-term)
		for (; *s1 == *s2; ++s1, ++s2)
		{
			if (*s1 == 0)
			{
				r = 0;
				break;
			}
		}

		// handle case where we didn't break early - set return code.
		if (r != 0)
		{
			r = *(const char*)s1 - *(const char*)s2;
		}
	}

	return r;
}

#endif
#endif

#if CUSTOM_IMPL
float floorf(float x)
{
	long r;
	r = x;
	if (r <= 0)
		return (r + ((r > x) ? -1 : 0));
	else
		return r;
}


#ifndef _WIN32
float powf(float a, float b)
{
	return __builtin_powf(a, b);
}
#endif

float frexpf(float x, int* e)
{
	union { float f; size_t i; } y = { x };
	int ee = y.i >> 23 & 0xff;
	if (!ee) {
		if (x) {
			x = frexpf(x * 0x1p64, e);
			*e -= 64;
		}
		else *e = 0;
		return x;
	}
	else if (ee == 0xff) {
		return x;
	}
	*e = ee - 0x7e;
	y.i &= 0x807ffffful;
	y.i |= 0x3f000000ul;
	return y.f;
}


char* strchr(const char* s, int c)
{
	do {
		if (*s == c)
		{
			return (char*)s;
		}
	} while (*s++);
	return (0);
}
char*
strpbrk(const char* s1, const char* s2)
{
	const char* scanp;
	int c, sc;
	while ((c = *s1++) != 0) {
		for (scanp = s2; (sc = *scanp++) != 0;)
			if (sc == c)
				return ((char*)(s1 - 1));
	}
	return 0;
}
#if 0


/*
	* PJ: my own strcmp implementation
	*
	* strcmp with short-circuit support: very common when you have const strings
	* combined by the compiler.
	* Otherwise we compare the strings as normal.
	* We bail out when s1 ends (null-term)
	*/

	/*-
	* Copyright (c) 2009 Xin LI <delphij@FreeBSD.org>
	* All rights reserved.
	*
	* Redistribution and use in source and binary forms, with or without
	* modification, are permitted provided that the following conditions
	* are met:
	* 1. Redistributions of source code must retain the above copyright
	*    notice, this list of conditions and the following disclaimer.
	* 2. Redistributions in binary form must reproduce the above copyright
	*    notice, this list of conditions and the following disclaimer in the
	*    documentation and/or other materials provided with the distribution.
	*
	* THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
	* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
	* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
	* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
	* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
	* SUCH DAMAGE.
	*/

#include <limits.h>
#include <string.h>

static const unsigned long mask01 = 0x01010101;
static const unsigned long mask80 = 0x80808080;
#define LONGPTR_MASK (sizeof(long) - 1)

/*
	* Helper macro to return string length if we caught the zero
	* byte.
	*/
#define testbyte(x)                                     \
do                                                  \
{                                                   \
	if(p[x] == '\0')                                \
		return ((size_t*)p - (size_t*)str + x); \
} while(0)

size_t strlen(const char* str)
{
	const char* p;
	const unsigned long* lp;

	/* Skip the first few bytes until we have an aligned p */
	for (p = str; (size_t)p & LONGPTR_MASK; p++)
	{
		if (*p == '\0')
		{
			return ((size_t*)p - (size_t*)str);
		}
	}

	/* Scan the rest of the string using word sized operation */
	// Cast to void to prevent alignment warning
	for (lp = (const unsigned long*)(const void*)p;; lp++)
	{
		if ((*lp - mask01) & mask80)
		{
			p = (const char*)(lp);
			testbyte(0);
			testbyte(1);
			testbyte(2);
			testbyte(3);
		}
	}

	/* NOTREACHED */
	// return (0);
}

#include <string.h>
int memcmp(const void* p1, const void* p2, size_t n)
{
	size_t i;

	/**
		* p1 and p2 are the same memory? easy peasy! bail out
		*/
	if (p1 == p2)
	{
		return 0;
	}

	if (!p1)
	{
		return 1;
	}

	if (!p2)
	{
		return -1;
	}

	// This for loop does the comparing and pointer moving...
	for (i = 0; (i < n) && (*(const unsigned char*)p1 == *(const unsigned char*)p2);
		i++, p1 = 1 + (const unsigned char*)p1, p2 = 1 + (const unsigned char*)p2)
	{
		// empty body
	}

	// if i == length, then we have passed the test
	return (i == n) ? 0 : (*(const unsigned char*)p1 - *(const unsigned char*)p2);
}


char* __strchrnul(const char*, int);

char* strchr(const char* s, int c)
{
	char* r = __strchrnul(s, c);
	return *(unsigned char*)r == (unsigned char)c ? r : 0;
}

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x)-ONES) & ~(x)&HIGHS)


//typedef unsigned int* uintptr_t;

char* __strchrnul(const char* s, int c)
{
	const size_t* w;
	size_t k;
	c = (unsigned char)c;

	if (!c)
	{
		return (char*)(uintptr_t)s + strlen(s);
	}

	for (; (size_t)s % ALIGN; s++)
	{
		if (!*s || *(const unsigned char*)s == c)
		{
			return (char*)(uintptr_t)s;
		}
	}

	k = ONES * (unsigned long)c;

	for (w = (const void*)s; !HASZERO(*w) && !HASZERO(*w ^ k); w++)
	{
		{
			;
		}
	}
	for (s = (const void*)w; *s && *(const unsigned char*)s != c; s++)
	{
		{
			;
		}
	}

	return (char*)(uintptr_t)s;
}
#endif
#endif
#ifdef __cplusplus
}
#endif
