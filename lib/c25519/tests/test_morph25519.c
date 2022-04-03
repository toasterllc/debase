/* Montgomery <-> Edwards isomorphism
 * Daniel Beer <dlbeer@gmail.com>, 18 Jan 2014
 *
 * This file is in the public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "morph25519.h"
#include "c25519.h"
#include "ed25519.h"

static void print_elem(const uint8_t *e)
{
	int i;

	for (i = 0; i < F25519_SIZE; i++)
		printf("%02x", e[i]);
}

static void test_morph(const uint8_t *mx,
		       const uint8_t *ex, const uint8_t *ey)
{
	const int parity = morph25519_eparity(ex);
	uint8_t mx_test[F25519_SIZE];
	uint8_t ex_test[F25519_SIZE];
	uint8_t ey_test[F25519_SIZE];

	printf("  ");
	print_elem(mx);
	printf(" [%d]\n    ~ (", parity);
	print_elem(ex);
	printf(", ");
	print_elem(ey);
	printf(")\n");

	morph25519_e2m(mx_test, ey);
	morph25519_m2e(ex_test, ey_test, mx, parity);

	assert(f25519_eq(mx_test, mx));
	assert(f25519_eq(ex_test, ex));
	assert(f25519_eq(ey_test, ey));
}

static void test_sm(void)
{
	uint8_t e[C25519_EXPONENT_SIZE];
	uint8_t mx[F25519_SIZE];
	uint8_t ex[F25519_SIZE];
	uint8_t ey[F25519_SIZE];
	struct ed25519_pt p;
	unsigned int i;

	for (i = 0; i < sizeof(e); i++)
		e[i] = random();

	c25519_prepare(e);
	c25519_smult(mx, c25519_base_x, e);

	ed25519_smult(&p, &ed25519_base, e);
	ed25519_unproject(ex, ey, &p);

	test_morph(mx, ex, ey);
}

int main(void)
{
	int i;

	srandom(0);

	printf("test_base\n");
	test_morph(c25519_base_x, ed25519_base.x, ed25519_base.y);

	printf("test_sm\n");
	for (i = 0; i < 32; i++)
		test_sm();

	return 0;
}
