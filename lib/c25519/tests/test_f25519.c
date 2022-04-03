/* Arithmetic mod p = 2^255-19
 * Daniel Beer <dlbeer@gmail.com>, 5 Jan 2014
 *
 * This file is in the public domain.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "f25519.h"

static void test_normalize_small(void)
{
	uint8_t e[F25519_SIZE];
	uint8_t f[F25519_SIZE];
	unsigned int i;

	for (i = 0; i < sizeof(e); i++)
		e[i] = random();
	e[31] &= 63;

	f25519_copy(f, e);
	f25519_normalize(f);

	assert(f25519_eq(f, e) == 1);
}

static void test_normalize_big(void)
{
	uint8_t e[F25519_SIZE];
	uint8_t f[F25519_SIZE];
	unsigned int i;

	for (i = 0; i < sizeof(e); i++)
		e[i] = random();
	e[31] |= 128;

	f25519_copy(f, e);
	f25519_normalize(f);

	assert(f25519_eq(f, e) == 0);

	f25519_copy(e, f);
	f25519_normalize(e);
	assert(f25519_eq(f, e) == 1);
}

static void test_normalize_gap(int k)
{
	uint8_t e[F25519_SIZE];
	unsigned int i;

	/* Construct p + k, where k < 19 */
	memset(e, 0xff, sizeof(e));
	e[31] &= 127;
	e[0] = k - 19;

	f25519_normalize(e);

	/* We should have k */
	assert(e[0] == k);
	for (i = 1; i < sizeof(e); i++)
		assert(!e[i]);
}

static void randomize(uint8_t *x)
{
	unsigned int i;

	for (i = 0; i < F25519_SIZE; i++)
		x[i] = random();
}

static void test_add_sub(void)
{
	uint8_t a[F25519_SIZE];
	uint8_t b[F25519_SIZE];
	uint8_t c[F25519_SIZE];
	uint8_t x[F25519_SIZE];

	randomize(a);
	randomize(b);
	randomize(c);

	/* Assumed to be less than 2p */
	c[31] &= 127;
	a[31] &= 127;

	f25519_add(x, a, b);
	f25519_sub(x, x, c);
	f25519_sub(x, x, a);
	f25519_add(x, x, c);

	f25519_normalize(x);
	f25519_normalize(b);
	assert(f25519_eq(x, b));
}

static void test_mul_c(void)
{
	uint8_t a[F25519_SIZE];
	uint8_t b[F25519_SIZE];
	uint8_t c[F25519_SIZE];

	randomize(a);

	f25519_add(b, a, a);
	f25519_mul_c(c, a, 2);

	f25519_normalize(b);
	f25519_normalize(c);
	assert(f25519_eq(b, c));
}

static void test_mul(void)
{
	uint8_t a[F25519_SIZE];
	uint8_t b[F25519_SIZE];
	uint8_t c[F25519_SIZE];
	uint8_t d[F25519_SIZE];
	uint8_t e[F25519_SIZE];
	uint32_t x = random();

	randomize(a);
	x = random() & 0xffffff;
	f25519_load(b, x);

	f25519_mul_c(c, a, x);
	f25519_mul__distinct(d, a, b);
	f25519_mul(e, a, b);

	f25519_normalize(c);
	f25519_normalize(d);
	f25519_normalize(e);

	assert(f25519_eq(c, d));
	assert(f25519_eq(d, e));
}

static void test_distributive(void)
{
	uint8_t a[F25519_SIZE];
	uint8_t b[F25519_SIZE];
	uint8_t x[F25519_SIZE];
	uint8_t e[F25519_SIZE];
	uint8_t f[F25519_SIZE];

	randomize(a);
	randomize(b);
	randomize(x);

	/* x*a + x*b */
	f25519_mul__distinct(e, a, x);
	f25519_mul__distinct(f, b, x);
	f25519_add(e, e, f);

	/* x*(a+b) */
	f25519_add(f, a, b);
	f25519_mul(f, f, x);

	f25519_normalize(e);
	f25519_normalize(f);
	assert(f25519_eq(e, f));
}

static void test_sqrt(void)
{
	uint8_t x[F25519_SIZE];
	uint8_t y[F25519_SIZE];
	uint8_t z1[F25519_SIZE];
	uint8_t z2[F25519_SIZE];
	uint8_t y1[F25519_SIZE];
	uint8_t y2[F25519_SIZE];

	randomize(x);

	f25519_mul__distinct(y, x, x);

	f25519_sqrt(z1, y);
	f25519_neg(z2, z1);

	f25519_mul__distinct(y1, z1, z1);
	f25519_mul__distinct(y2, z2, z2);

	f25519_normalize(x);
	f25519_normalize(y);
	f25519_normalize(z1);
	f25519_normalize(z2);
	f25519_normalize(y1);
	f25519_normalize(y2);

	assert(f25519_eq(y, y1));
	assert(f25519_eq(y, y2));
	assert(!f25519_eq(z1, z2));
	assert(f25519_eq(x, z1) | f25519_eq(x, z2));
}

static void test_inv(void)
{
	uint8_t a[F25519_SIZE];
	uint8_t ai[F25519_SIZE];
	uint8_t p[F25519_SIZE];
	uint8_t one[F25519_SIZE];

	randomize(a);
	f25519_load(one, 1);

	f25519_inv__distinct(ai, a);
	f25519_mul__distinct(p, a, ai);

	f25519_normalize(p);
	assert(f25519_eq(p, one));
}

int main(void)
{
	int i;

	printf("test_normalize_small\n");
	for (i = 0; i < 100; i++)
		test_normalize_small();

	printf("test_normalize_big\n");
	for (i = 0; i < 100; i++)
		test_normalize_big();

	printf("test_normalize_gap\n");
	for (i = 0; i < 19; i++)
		test_normalize_gap(i);

	printf("test_add_sub\n");
	for (i = 0; i < 100; i++)
		test_add_sub();

	printf("test_mul_c\n");
	for (i = 0; i < 100; i++)
		test_mul_c();

	printf("test_mul\n");
	for (i = 0; i < 100; i++)
		test_mul();

	printf("test_distributive\n");
	for (i = 0; i < 100; i++)
		test_distributive();

	printf("test_inv\n");
	for (i = 0; i < 100; i++)
		test_inv();

	printf("test_sqrt\n");
	for (i = 0; i < 100; i++)
		test_sqrt();

	return 0;
}
