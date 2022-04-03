/* Arithmetic in prime fields
 * Daniel Beer <dlbeer@gmail.com>, 10 Jan 2014
 *
 * This file is in the public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fprime.h"

/* 2^252 + 27742317777372353535851937790883648493 */
static const uint8_t modulus[FPRIME_SIZE] = {
	0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
	0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
};

static void randomize(uint8_t *x)
{
	int i;

	memset(x, 0, FPRIME_SIZE);

	for (i = 0; i < 32; i++)
		x[i] = random();
	x[31] &= 15;
}

static void test_normalize_zero(void)
{
	uint8_t t[FPRIME_SIZE];
	uint8_t zero[FPRIME_SIZE];

	memset(zero, 0, sizeof(zero));
	fprime_copy(t, modulus);

	assert(!fprime_eq(t, zero));

	fprime_normalize(t, modulus);

	assert(fprime_eq(t, zero));
}

static void test_normalize_small(void)
{
	uint8_t e[FPRIME_SIZE] = {0};
	uint8_t f[FPRIME_SIZE] = {0};

	randomize(e);

	fprime_copy(f, e);
	fprime_normalize(f, modulus);

	assert(fprime_eq(f, e) == 1);
}

static void test_normalize_big(void)
{
	uint8_t e[FPRIME_SIZE];
	uint8_t f[FPRIME_SIZE];
	unsigned int i;

	for (i = 0; i < sizeof(e); i++)
		e[i] = random();
	e[31] |= 128;

	fprime_copy(f, e);
	fprime_normalize(f, modulus);

	fprime_copy(e, f);
	fprime_normalize(e, modulus);

	assert(fprime_eq(f, e) == 1);
}

static void test_add_sub(void)
{
	uint8_t a[FPRIME_SIZE];
	uint8_t b[FPRIME_SIZE];
	uint8_t c[FPRIME_SIZE];
	uint8_t x[FPRIME_SIZE];

	randomize(a);
	randomize(b);
	randomize(c);

	/* Assumed to be less than 2p */
	c[31] &= 127;
	a[31] &= 127;

	fprime_copy(x, a);
	fprime_add(x, b, modulus);
	fprime_sub(x, c, modulus);
	fprime_sub(x, a, modulus);
	fprime_add(x, c, modulus);

	assert(fprime_eq(x, b));
}

static void test_mul(void)
{
	const uint32_t a = random() & 0xffff;
	const uint32_t b = random() & 0xffff;
	uint8_t fa[FPRIME_SIZE];
	uint8_t fb[FPRIME_SIZE];
	uint8_t fc[FPRIME_SIZE];
	uint8_t fd[FPRIME_SIZE];

	fprime_load(fa, a);
	fprime_load(fb, b);
	fprime_load(fc, a * b);
	fprime_mul(fd, fa, fb, modulus);

	assert(fprime_eq(fc, fd));
}

static void test_distributive(void)
{
	uint8_t a[FPRIME_SIZE];
	uint8_t b[FPRIME_SIZE];
	uint8_t x[FPRIME_SIZE];
	uint8_t e[FPRIME_SIZE];
	uint8_t f[FPRIME_SIZE];
	uint8_t g[FPRIME_SIZE];

	randomize(a);
	randomize(b);
	randomize(x);

	/* x*a + x*b */
	fprime_mul(e, a, x, modulus);
	fprime_mul(f, b, x, modulus);
	fprime_add(e, f, modulus);

	/* x*(a+b) */
	fprime_copy(g, a);
	fprime_add(g, b, modulus);
	fprime_mul(f, g, x, modulus);

	assert(fprime_eq(e, f));
}

static void test_inv(void)
{
	uint8_t a[FPRIME_SIZE];
	uint8_t ai[FPRIME_SIZE];
	uint8_t p[FPRIME_SIZE];
	uint8_t one[FPRIME_SIZE];

	randomize(a);

	fprime_load(one, 1);
	fprime_inv(ai, a, modulus);
	fprime_mul(p, a, ai, modulus);

	assert(fprime_eq(p, one));
}

static void test_from_bytes(void)
{
	uint8_t direct[100] = {0};
	uint8_t x[FPRIME_SIZE] = {1, 0};
	unsigned int i;

	for (i = 0; i < sizeof(direct) * 8; i++) {
		uint8_t y[FPRIME_SIZE];

		/* Create 2**i directly */
		direct[i >> 3] |= (1 << (i & 7));
		fprime_from_bytes(y, direct, sizeof(direct), modulus);
		direct[i >> 3] = 0;

		assert(fprime_eq(x, y));

		/* Double x */
		fprime_copy(y, x);
		fprime_add(x, y, modulus);
	}
}

int main(void)
{
	unsigned int i;

	printf("test_normalize_zero\n");
		test_normalize_zero();

	printf("test_normalize_small\n");
	for (i = 0; i < 100; i++)
		test_normalize_small();

	printf("test_normalize_big\n");
	for (i = 0; i < 100; i++)
		test_normalize_big();

	printf("test_from_bytes\n");
	test_from_bytes();

	printf("test_add_sub\n");
	for (i = 0; i < 100; i++)
		test_add_sub();

	printf("test_mul\n");
	for (i = 0; i < 100; i++)
		test_mul();

	printf("test_distributive\n");
	for (i = 0; i < 100; i++)
		test_distributive();

	printf("test_inv\n");
	for (i = 0; i < 10; i++)
		test_inv();

	return 0;
}
