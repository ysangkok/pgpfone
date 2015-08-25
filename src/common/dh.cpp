#include <assert.h>
#include <string.h>	/* For memset */

#include "bn.h"
#include "bytefifo.h"
#include "dh.h"
#include "fastpool.h"

/* A 512-bit Diffie-Hellman prime */
unsigned char const dhPrime[512/8] = {
	0xC6, 0x00, 0x72, 0xFD, 0xBE, 0x59, 0xEB, 0x15,
	0xF6, 0x30, 0xE2, 0xAD, 0xAF, 0x89, 0x5B, 0xC4,
	0xE6, 0x61, 0x52, 0x5D, 0x9E, 0xB8, 0xCA, 0x75,
	0xD6, 0x91, 0xC2, 0x0D, 0x8F, 0xE8, 0x3B, 0x24,
	0xC6, 0xC0, 0x33, 0xBD, 0x7F, 0x19, 0xAB, 0xD5,
	0xB6, 0xF0, 0xA3, 0x6D, 0x6E, 0x49, 0x1A, 0x84,
	0xA7, 0x21, 0x13, 0x1D, 0x5F, 0x78, 0x8B, 0x35,
	0x97, 0x51, 0x82, 0xCD, 0x4E, 0xB3, 0xA4, 0x03
};

/* The generator */
unsigned char const dhGenerator[1] = { 2 };

/*
 * Number of most-significant bits of the modulus not to count as part
 * of the shared secret.  The number of significant bits of modulus
 * is counted, this is subtracted, and the result rounded down to the
 * nearest byte.  The remaining least-significant bytes of the result are
 * considered to be the shared secret.
 */
#define DERATING 4

/*
 * Generate a random bignum of a specified maximum length.
 */
static int
genRandBn(struct BigNum *bn, unsigned bits)
{
	unsigned char buf[64];
	unsigned bytes;
	unsigned l;
	int err;

	bnSetQ(bn, 0);

	bytes = (bits+7) / 8;
	l = bytes < sizeof(buf) ? bytes : sizeof(buf);
	randPoolGetBytes(buf, l);

	/* Mask off excess high bits */
	buf[0] &= 255 >> (-bits & 7);

	for (;;) {
		bytes -= l;
		err = bnInsertBigBytes(bn, buf, bytes, l);
		if (!bytes || err < 0)
			break;
		l = bytes < sizeof(buf) ? bytes : sizeof(buf);
		randPoolGetBytes(buf, l);
	}

	memset(buf, 0, sizeof(buf));
	return err;
}

/*
 * This function is separated out because of the size of comment it
 * requires.  There are two ways to solve the discrete log problem
 * (given y = g^x mod p, find x) that is at the heart of the
 * Diffie-Hellman algorithm's security.  One is the number field
 * sieve, which takes time that depends only on the size of p.
 * The other is Pollard's rho method and variants, which depend
 * mostly on the size of the exponent x.  (Both can be efficiently
 * parallelized, so you can easily throw thousands of computers
 * at the problem.)  There is no security benefit to choosing x
 * larger than the threshold at which the number field sieve is the
 * fastest of the two attacks, and larger choices of x do slow down
 * Diffie-Hellman operations.
 *
 * The best estimates we have right now of the threshold size of
 * x are those computed by Michael Wiener, in his paper "Balancing
 * Field Size and Subgroup Size in DSA" (the DSA is based on the same
 * mathematics as Diffie-Hellman, where p defines the "field" and x
 * is chosen from the "subgroup") which includes the following table:
 *
 *           Table 1: Subgroup Sizes to Match Field Sizes
 *
 *          Size of p    Cost of each attack     Size of q
 *           (bits)      (instructions or         (bits)
 *                        modular multiplies)
 *
 *             512            9 x 10^17             119
 *             768            6 x 10^21             145
 *            1024            7 x 10^24             165
 *            1280            3 x 10^27             183
 *            1536            7 x 10^29             198
 *            1792            9 x 10^31             212
 *            2048            8 x 10^33             225
 *            2304            5 x 10^35             237
 *            2560            3 x 10^37             249
 *            2816            1 x 10^39             259
 *            3072            3 x 10^40             269
 *            3328            8 x 10^41             279
 *            3584            2 x 10^43             288
 *            3840            4 x 10^44             296
 *            4096            7 x 10^45             305
 *            4352            1 x 10^47             313
 *            4608            2 x 10^48             320
 *            4864            2 x 10^49             328
 *            5120            3 x 10^50             335
 *
 * Based on this, the following code computes an approximation of
 * this table, as given below:
 *
 * Size of p  Approx.  Wiener  Error
 *    256	120
 *    512	137	119	+18
 *    768	153	145	+8
 *   1024	169	165	+4
 *   1280	184	183	+1
 *   1536	198	198	 0
 *   1792	212	212	 0
 *   2048	225	225	 0
 *   2304	237	237	 0
 *   2560	249	249	 0
 *   2816	260	259	+1
 *   3072	270	269	+1
 *   3328	280	279	+1
 *   3584	289	288	+1
 *   3840	297	296	+1
 *   4096	305	305	 0
 *   4352	313	313	 0
 *   4608	321	320	+1
 *   4864	329	328	+1
 *   5120	337	335	+2
 *   5376	345
 *   5632	353
 *   5888	361
 *   6144	369
 *   6400	377
 *
 * ... and then pads it somewhat, for safety, as increasing x does not
 * slow down Diffie-Hellman all that much.  The "safety" is against
 * improved attacks against the exponent x.  An improvement in an attack
 * against the modulus p would, in contrast, decrease the size of x
 * needed.
 *
 * The approximation is a combination of a linear and a quadratic
 * curve, with the junction point at (3840,297) and a slope of 8/256.
 * Below this, the quadratic approximation is used; above it, the
 * linear.  This entirely determines the linear approximation and leaves
 * only one free parameter in the quadratic, which was chosen as the
 * coefficient of x^2 and adjusted for good performance.
 */
#define AN      1       /* a = -AN/AD/65536, the quadratic coefficient */
#define AD      3
#define M       8       /* Slope = M/256, i.e. 1/32 where linear starts */
#define TX      3840    /* X value at the slope point, where linear starts */
#define TY      297     /* Y value at the slope point, where linear starts */
/*
 * The derivation of the formula proceeds as follows, starting from
 * y = a*(x-TX)^2 + M/256*(x-TX) + TY
 *   = -AN/AD*((x-TX)/256)^2 + M*(x-TX)/256 + TY
 *   = -AN*(x-TX)*(x-TX)/AD/256/256 + M*x/256 - M*TX/256 + TY
 *   = -AN*x*x/AD/256/256 + 2*AN*x*TX/AD/256/256 - AN*TX*TX/AD/256/256 \
 *      + M*x/256 - M*TX/256 + TY
 *   = -AN*(x/256)^2/AD + 2*AN*(TX/256)*(x/256)/AD + M*(x/256) \
 *      - AN*(TX/256)^2/AD - M*(TX/256) + TY
 *   = (AN*(2*TX/256 - x/256) + M*AD)*x/256/AD - (AN*(TX/256)/AD + M)*TX/256 \
 *      + TY
 *   = (AN*(2*TX/256 - x/256) + M*AD)*x/256/AD \
 *      - (AN*(TX/256) + M*AD)*TX/256/AD + TY
 *   =  ((M*AD + AN*(2*TX/256 - x/256))*x - (AN*(TX/256)+M*AD)*TX)/256/AD + TY
 *   =  ((M*AD + AN*(2*TX - x)/256)*x - (AN*(TX/256)+M*AD)*TX)/256/AD + TY
 *   =  ((M*AD + AN*(2*TX - x)/256)*x - (M*AD + AN*TX/256)*TX)/256/AD + TY
 *
 * Since this is for the range 0...TX, in order to avoid having any
 * intermediate results less than 0, we need one final rearrangement:
 *
 *   =  TY - ((M*AD + AN*TX/256)*TX - (M*AD + AN*(2*TX - x)/256)*x)/256/AD
 *   =  TY - ((M*AD + AN*TX/256)*TX - (256*M*AD + AN*(2*TX - x))/256*x)/256/AD
 *   =  TY - ((M*AD + AN*TX/256)*TX - (256*M*AD + AN*2*TX - AN*x)/256*x)/256/AD
 *
 * The formula is not very expensive to compute, because a compiler can
 * evaluate most of the expression beforehand and divides by 256 are shifts.
 * I.e. it ends up as 297 - (149760 - ((13824-x)/256)*x)/3/256.
 */
#define F(x) \
        TY - ((M*AD + (AN*TX)/256)*TX - (256*M*AD+AN*2*TX-AN*x)/256*x)/AD/256

static unsigned
dhExponentSize(unsigned pbits)
{
	unsigned ebits;

	if (pbits < TX)	/* 3456 is where the two are tangent */
		ebits = F(pbits);	/* Quadratic approximation */
	else
		ebits = M*pbits/256 - M*TX/256 + TY;    /* Linear */
	if(ebits <384)
		ebits = 384;
	return ebits + 32;	/* Padding for safety */
}

/*
 * Generate a random private value (of up to the specified length),
 * raise the generator to that power (modulo the modulus), and return
 * it in the "public" buffer (modlen bytes long).
 */
int
dhFirstHalf(unsigned char const *modulus, unsigned modlen, unsigned explen,
	unsigned char const *generator, unsigned genlen,
	unsigned char *oursecret, unsigned oursecretlen,
	unsigned char *ourpublic)
{
	struct BigNum m, priv, pub;
	unsigned bits;

	bnBegin(&m);
	bnBegin(&priv);
	bnBegin(&pub);

	if (bnInsertBigBytes(&m, modulus, 0, modlen) < 0)
		goto error;
	/* Load generator into pub */
	if (bnInsertBigBytes(&pub, generator, 0, genlen) < 0)
		goto error;
	pgpAssert(bnBits(&pub) > 1);

	/*
	 * Figure out how many bits of private value we want...
	 */
	bits = bnBits(&m);
	pgpAssert(bits >= 8);
	//bits = dhExponentSize(bits);
	bits = explen;		// hardcoded substitute for function
	/* Make sure the secret isn't too long for the buffer */
	if (bits > oursecretlen * 8)
		bits = oursecretlen * 8;
	if (genRandBn(&priv, bits) < 0)
		goto error;
	pgpAssert(bnBits(&priv) > bits/2);	/* Catastrophic RNG failure! */

	/* Write out the private data */
	bnExtractBigBytes(&priv, oursecret, 0, oursecretlen);

	/* Do the exponentiation. */
	if (bnExpMod(&pub, &pub, &priv, &m) < 0)
		goto error;
	/* Write out the result */
	bnExtractBigBytes(&pub, ourpublic, 0, modlen);

	bnEnd(&m);
	bnEnd(&priv);
	bnEnd(&pub);
	return 0;
	
error:
	bnEnd(&m);
	bnEnd(&priv);
	bnEnd(&pub);
	return -1;
}

/*
 * Write "bytes" least-significant bytes of the given bignum,
 * in big-endian order, to the given byte fifo.  Returns 0 on
 * success, -1 on an out-of-memory error.
 */
static int
writeBnFifoBytes(struct BigNum const *bn, struct ByteFifo *fifo, unsigned bytes)
{
	unsigned char *p;
	ulong avail;

	while ((p = byteFifoGetSpace(fifo, &avail)), avail < bytes) {
		bnExtractBigBytes(bn, p, bytes-avail, avail);
		byteFifoSkipSpace(fifo, avail);
	}
	if (!p)
		return -1;
	bnExtractBigBytes(bn, p, 0, bytes);
	byteFifoSkipSpace(fifo, bytes);
	return 0;
}

/*
 * Write the specified bignum to the given fifo, in big-endian order with
 * leading 0 bytes suppressed, preceded by a 2-byte big-endian byte count.
 */
static int
writeBnFifo(struct BigNum const *bn, struct ByteFifo *fifo)
{
	unsigned char buf[2];
	unsigned bytes;

	bytes = (bnBits(bn) + 7)/8;
	pgpAssert(bytes <= 0xffff);
	buf[0] = (unsigned char)(bytes >> 8);
	buf[1] = (unsigned char)bytes;

	if (byteFifoWrite(fifo, buf, 2) != 2)
		return -1;
	memset(buf, 0, sizeof(buf));

	return writeBnFifoBytes(bn, fifo, bytes);
}

/*
 * Given a remote value coming from the other side of a Diffie-Hellman
 * key agreement, raise it to the power of "oursecret" and use the
 * result as a shared secret.  The resulting shared secret is stored
 * in the SharedSecretFifo, and authentication data is stored in the
 * AuthenticationFifo.
 * Returns 0 on success, or -1 on error (out of memory).
 */
int
dhSecondHalf(unsigned char const *modulus, unsigned modlen,
	unsigned char const *generator, unsigned genlen,
	unsigned char const *oursecret, unsigned oursecretlen,
	unsigned char const *theirpublic, unsigned theirpubliclen,
	struct ByteFifo *SharedSecretFifo, struct ByteFifo *AuthenticationFifo)
{
	struct BigNum m, priv, rem;
	unsigned len;
	static unsigned char const dhsig[] = { 1 };

	bnBegin(&m);
	bnBegin(&priv);
	bnBegin(&rem);

	if (bnInsertBigBytes(&m, modulus, 0, modlen) < 0)
		goto error;
	/* Load their public value */
	if (bnInsertBigBytes(&rem, theirpublic, 0, theirpubliclen) < 0)
		goto error;
	if (bnMod(&rem, &rem, &m) < 0)
		goto error;
	/*
	 * Check that "rem" isn't too close to 0 (+ *or* -),
	 * lest some active eavesdropper try an attack.
	 */
	len = bnBits(&m);
	bnCopy(&priv, &m);
	bnSub(&priv, &rem);
	if (bnBits(&rem) < len/2 || bnBits(&priv) < len/2)
		goto error;	/* XXX improve this error message */
	/* Load private value */
	if (bnInsertBigBytes(&priv, oursecret, 0, oursecretlen) < 0)
		goto error;

	/* Do the exponentiation. */
	if (bnExpMod(&rem, &rem, &priv, &m) < 0)
		goto error;

	/*
	 * Now that success is guaranteed, write out the data.
	 * First, to the shared secret FIFO...
	 */
	pgpAssert(len > DERATING + 8);
	len = (len - DERATING) / 8;	/* Now it's a byte count */
	if (writeBnFifoBytes(&rem, SharedSecretFifo, len) < 0)
		goto error;
	if(AuthenticationFifo)
	{
		/*
		 * And then to the authentication FIFO.
		 * Format for Diffie-Hellman authentication data:
		 * - Diffie-Hellman signature header (must be unique)
		 * - Prime modulus
		 * - Generator
		 * - Shared secret (all of it, not derated)
		 */
		len = sizeof(dhsig);
		if (byteFifoWrite(AuthenticationFifo, dhsig, len) != len)
			goto error;
		if (writeBnFifo(&m, AuthenticationFifo) < 0)
			goto error;
		(void*)bnSetQ(&m, 0);
		if (bnInsertBigBytes(&m, generator, 0, genlen) < 0)
			goto error;
		if (writeBnFifo(&m, AuthenticationFifo) < 0)
			goto error;
		if (writeBnFifo(&rem, AuthenticationFifo) < 0)
			goto error;
	}
	bnEnd(&m);
	bnEnd(&priv);
	bnEnd(&rem);
	return 0;
	
error:
	bnEnd(&m);
	bnEnd(&priv);
	bnEnd(&rem);
	return -1;
}

