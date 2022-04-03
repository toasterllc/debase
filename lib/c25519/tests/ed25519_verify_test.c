#ifdef SODIUM
#include "crypto_sign_ed25519.h"
#else
#include "edsign.h"
#endif
#include <unistd.h>
#include <stdio.h>
#include "hexin.h"

// fields on each input line: sk, pk, m, sm
// each field hex
// each field colon-terminated
// sk includes pk at end
// sm includes m at end

int main(void)
{
	unsigned char sk[200];  // secret key plus public key (64 octets)
	unsigned char pk[100];  // public key (32 octets)
	unsigned char m[1100];  // message
	unsigned char sm[1200]; // signature (64 octets) plus message
	unsigned char gb[10];   // garbage

	int rc=0;
	for (;;) {
		ssize_t sk_l = get_from_hex(sk, sizeof sk, ':');
		ssize_t pk_l = get_from_hex(pk, sizeof pk, ':');
		ssize_t m_l  = get_from_hex(m,  sizeof m,  ':');
		ssize_t sm_l = get_from_hex(sm, sizeof sm, ':');
		/*          */ get_from_hex(sm, sizeof gb, '\n');
		/* printf("%d %d %d %d\n", (int) sk_l, (int) pk_l, (int) m_l, (int) sm_l); */
		if (sk_l != 64 || pk_l != 32 || m_l < 0 || sm_l != m_l+64) break;
		// uncomment the next line to test response of the test to failure
		// if (m_l == 27) m[10]++;
#ifdef SODIUM
		int r = crypto_sign_ed25519_verify_detached(sm, m, m_l, pk);
#else
		int r = !edsign_verify(sm, pk, m, m_l);
#endif
		if (r!=0) {
			printf("%d: %s\n", (int) m_l, r==0 ? "OK" : "FAIL");
			rc = 2;
		}
	}
	return rc;
}
