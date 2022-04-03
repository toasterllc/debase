/* Edwards curve operations
 * Daniel Beer <dlbeer@gmail.com>, 9 Jan 2014
 *
 * This file is in the public domain.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "ed25519.h"
#include "c25519.h"

static const uint8_t ed25519_order[ED25519_EXPONENT_SIZE] = {
	0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
	0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
};

static const uint8_t curve_d[F25519_SIZE] = {
	0xa3, 0x78, 0x59, 0x13, 0xca, 0x4d, 0xeb, 0x75,
	0xab, 0xd8, 0x41, 0x41, 0x4d, 0x0a, 0x70, 0x00,
	0x98, 0xe8, 0x79, 0x77, 0x79, 0x40, 0xc7, 0x8c,
	0x73, 0xfe, 0x6f, 0x2b, 0xee, 0x6c, 0x03, 0x52
};

static void print_point(const uint8_t *x, const uint8_t *y)
{
	int i;

	printf("  ");
	for (i = 0; i < F25519_SIZE; i++)
		printf("%02x", x[i]);
	printf(", ");
	for (i = 0; i < F25519_SIZE; i++)
		printf("%02x", y[i]);
	printf("\n");
}

static void check_valid(const uint8_t *x, const uint8_t *y)
{
	uint8_t xsq[F25519_SIZE];
	uint8_t ysq[F25519_SIZE];
	uint8_t m[F25519_SIZE];
	uint8_t dxsqysq[F25519_SIZE];
	uint8_t lhs[F25519_SIZE];
	uint8_t rhs[F25519_SIZE];

	f25519_mul__distinct(xsq, x, x);
	f25519_mul__distinct(ysq, y, y);
	f25519_mul__distinct(m, xsq, ysq);
	f25519_mul__distinct(dxsqysq, m, curve_d);

	f25519_sub(lhs, ysq, xsq);
	f25519_load(rhs, 1);
	f25519_add(rhs, rhs, dxsqysq);

	f25519_normalize(lhs);
	f25519_normalize(rhs);

	assert(f25519_eq(lhs, rhs));
}

static void test_pack(void)
{
	uint8_t e[ED25519_EXPONENT_SIZE];
	uint8_t x1[F25519_SIZE];
	uint8_t y1[F25519_SIZE];
	uint8_t pack[F25519_SIZE];
	uint8_t x2[F25519_SIZE];
	uint8_t y2[F25519_SIZE];
	struct ed25519_pt p;
	uint8_t s;
	int i;

	for (i = 0; i < ED25519_EXPONENT_SIZE; i++)
		e[i] = random();

	ed25519_smult(&p, &ed25519_base, e);
	ed25519_unproject(x1, y1, &p);
	f25519_normalize(x1);
	f25519_normalize(y1);

	check_valid(x1, y1);

	ed25519_pack(pack, x1, y1);
	s = ed25519_try_unpack(x2, y2, pack);

	assert(s);
	assert(f25519_eq(x1, x2));
	assert(f25519_eq(y1, y2));

	print_point(x1, y1);
}

static void test_dh(void)
{
	uint8_t e1[ED25519_EXPONENT_SIZE];
	uint8_t e2[ED25519_EXPONENT_SIZE];
	uint8_t p1x[F25519_SIZE];
	uint8_t p1y[F25519_SIZE];
	uint8_t p2x[F25519_SIZE];
	uint8_t p2y[F25519_SIZE];
	uint8_t s1x[F25519_SIZE];
	uint8_t s1y[F25519_SIZE];
	uint8_t s2x[F25519_SIZE];
	uint8_t s2y[F25519_SIZE];
	struct ed25519_pt p;
	int i;

	for (i = 0; i < ED25519_EXPONENT_SIZE; i++) {
		e1[i] = random();
		e2[i] = random();
	}

	/* Generate public key 1 */
	ed25519_smult(&p, &ed25519_base, e1);
	ed25519_unproject(p1x, p1y, &p);

	/* Generate public key 2 */
	ed25519_smult(&p, &ed25519_base, e2);
	ed25519_unproject(p2x, p2y, &p);

	/* Generate shared secret 1 */
	ed25519_project(&p, p1x, p1y);
	ed25519_smult(&p, &p, e2);
	ed25519_unproject(s1x, s1y, &p);

	/* Generate shared secret 2 */
	ed25519_project(&p, p2x, p2y);
	ed25519_smult(&p, &p, e1);
	ed25519_unproject(s2x, s2y, &p);

	/* Should be the same point */
	f25519_normalize(s1x);
	f25519_normalize(s1y);
	f25519_normalize(s2x);
	f25519_normalize(s2y);

	assert(f25519_eq(s1x, s2x));
	assert(f25519_eq(s1y, s2y));

	print_point(s1x, s1y);
}

static void test_add(void)
{
	struct ed25519_pt p;
	uint8_t ax[F25519_SIZE];
	uint8_t ay[F25519_SIZE];
	uint8_t bx[F25519_SIZE];
	uint8_t by[F25519_SIZE];

	printf("  double\n");
	ed25519_double(&p, &ed25519_base);
	ed25519_unproject(ax, ay, &p);
	check_valid(ax, ay);

	printf("  add\n");
	ed25519_add(&p, &ed25519_neutral, &ed25519_base);
	ed25519_add(&p, &p, &ed25519_base);
	ed25519_unproject(bx, by, &p);
	check_valid(bx, by);

	printf("  eq\n");
	f25519_normalize(ax);
	f25519_normalize(ay);
	f25519_normalize(bx);
	f25519_normalize(by);

	assert(f25519_eq(ax, bx));
	assert(f25519_eq(ay, by));
}

static void test_order(void)
{
	static const uint8_t zero[ED25519_EXPONENT_SIZE] = {0};
	uint8_t x[F25519_SIZE];
	uint8_t y[F25519_SIZE];
	uint8_t b1[F25519_SIZE];
	uint8_t b2[F25519_SIZE];
	struct ed25519_pt p;

	ed25519_smult(&p, &ed25519_base, ed25519_order);
	ed25519_unproject(x, y, &p);
	ed25519_pack(b1, x, y);

	ed25519_smult(&p, &ed25519_base, zero);
	ed25519_unproject(x, y, &p);
	ed25519_pack(b2, x, y);

	assert(f25519_eq(b1, b2));
}

int main(void)
{
	int i;

	srandom(0);

	printf("check_valid(ed25519_neutral)\n");
	check_valid(ed25519_neutral.x, ed25519_neutral.y);

	printf("check_valid(ed25519_base)\n");
	check_valid(ed25519_base.x, ed25519_base.y);

	printf("test_double_add\n");
	test_add();

	printf("test_order\n");
	test_order();

	printf("test_smult_pack\n");
	for (i = 0; i < 20; i++)
		test_pack();

	printf("test_dh\n");
	for (i = 0; i < 10; i++)
		test_dh();

	return 0;
}
