/*
 * Digital Signature Algorithm prime generation using the bignum
 * library and sieving.
 *
 * Written by Colin Plumb.
 *
 * $Id: bndsaprime.c,v 1.1 1997/12/14 11:30:12 wprice Exp $
 */
#include <stdarg.h>
#include <string.h>
#if BNDEBUG
#include <stdio.h>
#endif
#include "bnkludge.h"

#include "bn.h"
#include "bnimem.h"	/* For bniMemWipe */
#include "bndsaprime.h"
#include "bnsieve.h"
#include "pgpDebug.h"

/* Size of the sieve area (can be up to 65536/8 = 8192) */
#define SIEVE 1024

/*
 * Helper function that does the slow primality test.
 * bn is the input bignum; a and e are temporary buffers that are
 * allocated by the caller to save overhead.
 *
 * Returns 0 if prime, >0 if not prime, and -1 on error (out of memory).
 * If not prime, the return value is the number of modular exponentiations
 * performed.   Calls the progress function with progress indicators as
 * tests are passed.
 *
 * Currently, the testing performs Euler tests, to the bases 2, 3, 5, 7,
 * 11, 13 and 17.  (An Euler test is a slightly strengthened Fermat test;
 * Euler pseudoprimes are a subset of Fermat pseudoprimes.)  Some people
 * worry that this might not be enough.  Number theorists may wish to
 * generate primality proofs, but for random inputs, this returns non-primes
 * with a probability which is quite negligible, which is good enough.
 * NOTE that for NON-random inputs, there are indeed numbers which are
 * strong pseudoprimes to a lot of bases.  Up to 1/4 of all bases 0 < b < n.
 *
 * It has been proved (see Carl Pomerance, "On the Distribution of
 * Pseudoprimes", Math. Comp. v.37 (1981) pp. 587-593) that the number
 * of pseudoprimes (composite numbers that pass a Fermat test to the
 * base 2) less than x is bounded by:
 * exp(ln(x)^(5/14)) <= P_2(x)	### CHECK THIS FORMULA - it looks wrong! ###
 * P_2(x) <= x * exp(-1/2 * ln(x) * ln(ln(ln(x))) / ln(ln(x))).
 * Thus, the local density of Pseudoprimes near x is at most
 * exp(-1/2 * ln(x) * ln(ln(ln(x))) / ln(ln(x))), and at least
 * exp(ln(x)^(5/14) - ln(x)).  Here are some values of this function
 * for various k-bit numbers x = 2^k:
 * Bits	Density <=	Bit equivalent	Density >=	Bit equivalent
 *  128	3.577869e-07	 21.414396	4.202213e-37	 120.840190
 *  192	4.175629e-10	 31.157288	4.936250e-56	 183.724558
 *  256 5.804314e-13	 40.647940	4.977813e-75	 246.829095
 *  384 1.578039e-18	 59.136573	3.938861e-113	 373.400096
 *  512 5.858255e-24	 77.175803	2.563353e-151	 500.253110
 *  768 1.489276e-34	112.370944	7.872825e-228	 754.422724
 * 1024 6.633188e-45	146.757062	1.882404e-304	1008.953565
 *
 * As you can see, there's quite a bit of slop between these estimates.
 * In fact, the density of pseudoprimes is conjectured to be closer
 * to the square of that upper bound.  E.g. the density of pseudoprimes
 * of size 256 is around 3 * 10^-27.  The density of primes is very
 * high, from 0.005636 at 256 bits to 0.001409 at 1024 bits, i.e.
 * more than 10^-3.
 *
 * For those people used to cryptographic levels of security where the
 * 56 bits of DES key space is too small because it's exhaustible with
 * custom hardware searching engines, note that you are not generating
 * 50,000,000 primes per second on each of 56,000 custom hardware chips
 * for several hours.  The chances that another Dinosaur Killer asteroid
 * will land today is about 10^-11 or 2^-36, so it would be better to
 * spend your time worrying about *that*.  Well, okay, there should be
 * some derating for the chance that astronomers haven't seen it yet, but
 * I think you get the idea.  For a good feel about the probability of
 * various events, I have heard that a good book is by E'mile Borel,
 * "Les Probabilite's et la vie".  (The 's are accents, not apostrophes.)
 *
 * For more on the subject, try "Finding Four Million Large Random Primes",
 * by Ronald Rivest, in Advancess in Cryptology: Proceedings of Crypto '90.
 * He used a small-divisor test, then a Fermat test to the base 2, and then
 * 8 iterations of a Miller-Rabin test.  About 718 million random 256-bit
 * integers were generated, 43,741,404 passed the small divisor test,
 * 4,058,000 passed the Fermat test, and all 4,058,000 passed all 8
 * iterations of the Miller-Rabin test, proving their primality beyond most
 * reasonable doubts.
 *
 * If the probability of getting a pseudoprime is some small p, then
 * the probability of not getting it in t trials is (1-p)^t.  Remember
 * that, for small p, (1-p)^(1/p) ~ 1/e, the base of natural logarithms.
 * (This is more commonly expressed as e = lim_{x\to\infty} (1+1/x)^x.)
 * Thus, (1-p)^t ~ e^(-p*t) = exp(-p*t).  So the odds of being able to
 * do this many tests without seeing a pseudoprime if you assume that
 * p = 10^-6 (one in a million) is one in 57.86.  If you assume that
 * p = 2*10^-6, it's one in 3347.6.  So it's implausible that the
 * density of pseudoprimes is much more than one millionth the density
 * of primes.
 *
 * He also gives a theoretical argument that the chance of finding a
 * 256-bit non-prime which satisfies one Fermat test to the base 2 is less
 * than 10^-22.  The small divisor test improves this number, and if the
 * numbers are 512 bits (as needed for a 1024-bit key) the odds of failure
 * shrink to about 10^-44.  Thus, he concludes, for practical purposes
 * *one* Fermat test to the base 2 is sufficient.
 */
static int
primeTest(BigNum const *bn, BigNum *e, BigNum *a,
	int (*f)(void *arg, int c), void *arg)
{
	int err;
	unsigned i, j;
	unsigned k, l;
	static unsigned const primes[] = {2, 3, 5, 7, 11, 13, 17};
#define CONFIRMTESTS 7

#if BNDEBUG	/* Debugging */
	/*
	 * This is debugging code to test the sieving stage.
	 * If the sieving is wrong, it will let past numbers with
	 * small divisors.  The prime test here will still work, and
	 * weed them out, but you'll be doing a lot more slow tests,
	 * and presumably excluding from consideration some other numbers
	 * which might be prime.  This check just verifies that none
	 * of the candidates have any small divisors.  If this
	 * code is enabled and never triggers, you can feel quite
	 * confident that the sieving is doing its job.
	 */
	i = bnModQ(bn, 30030);	/* 30030 = 2 * 3 * 5 * 7 * 11 * 13 */
	if (!(i % 2)) printf("bn div by 2!");
	if (!(i % 3)) printf("bn div by 3!");
	if (!(i % 5)) printf("bn div by 5!");
	if (!(i % 7)) printf("bn div by 7!");
	if (!(i % 11)) printf("bn div by 11!");
	if (!(i % 13)) printf("bn div by 13!");
	i = bnModQ(bn, 7429);	/* 7429 = 17 * 19 * 23 */
	if (!(i % 17)) printf("bn div by 17!");
	if (!(i % 19)) printf("bn div by 19!");
	if (!(i % 23)) printf("bn div by 23!");
	i = bnModQ(bn, 33263);	/* 33263 = 29 * 31 * 37 */
	if (!(i % 29)) printf("bn div by 29!");
	if (!(i % 31)) printf("bn div by 31!");
	if (!(i % 37)) printf("bn div by 37!");
#endif

	/*
	 * Now, check that bn is prime.  If it passes to the base 2,
	 * it's prime beyond all reasonable doubt, and everything else
	 * is just gravy, but it gives people warm fuzzies to do it.
	 *
	 * This starts with verifying Euler's criterion for a base of 2.
	 * This is the fastest pseudoprimality test that I know of,
	 * saving a modular squaring over a Fermat test, as well as
	 * being stronger.  7/8 of the time, it's as strong as a strong
	 * pseudoprimality test, too.  (The exception being when bn ==
	 * 1 mod 8 and 2 is a quartic residue, i.e. bn is of the form
	 * a^2 + (8*b)^2.)  The precise series of tricks used here is
	 * not documented anywhere, so here's an explanation.
	 * Euler's criterion states that if p is prime then a^((p-1)/2)
	 * is congruent to Jacobi(a,p), modulo p.  Jacobi(a,p) is
	 * a function which is +1 if a is a square modulo p, and -1 if
	 * it is not.  For a = 2, this is particularly simple.  It's
	 * +1 if p == +/-1 (mod 8), and -1 if m == +/-3 (mod 8).
	 * If p == 3 mod 4, then all a strong test does is compute
	 * 2^((p-1)/2). and see if it's +1 or -1.  (Euler's criterion
	 * says *which* it should be.)  If p == 5 (mod 8), then
	 * 2^((p-1)/2) is -1, so the initial step in a strong test,
	 * looking at 2^((p-1)/4), is wasted - you're not going to
	 * find a +/-1 before then if it *is* prime, and it shouldn't
	 * have either of those values if it isn't.  So don't bother.
	 *
	 * The remaining case is p == 1 (mod 8).  In this case, we
	 * expect 2^((p-1)/2) == 1 (mod p), so we expect that the
	 * square root of this, 2^((p-1)/4), will be +/-1 (mod p).
	 * Evaluating this saves us a modular squaring 1/4 of the time.
	 * If it's -1, a strong pseudoprimality test would call p
	 * prime as well.  Only if the result is +1, indicating that
	 * 2 is not only a quadratic residue, but a quartic one as well,
	 * does a strong pseudoprimality test verify more things than
	 * this test does.  Good enough.
	 *
	 * We could back that down another step, looking at 2^((p-1)/8)
	 * if there was a cheap way to determine if 2 were expected to
	 * be a quartic residue or not.  Dirichlet proved that 2 is
	 * a quartic residue iff p is of the form a^2 + (8*b^2).
	 * All primes == 1 (mod 4) can be expressed as a^2 + (2*b)^2,
	 * but I see no cheap way to evaluate this condition.
	 */
	if (bnCopy(e, bn) < 0)
		return -1;
	(void)bnSubQ(e, 1);
	l = bnLSWord(e);

	j = 1;	/* Where to start in prime array for strong prime tests */

	if (l & 7) {
		bnRShift(e, 1);
		if (bnTwoExpMod(a, e, bn) < 0)
			return -1;
		if ((l & 7) == 6) {
			/* bn == 7 mod 8, expect +1 */
			if (bnBits(a) != 1)
				return 1;	/* Not prime */
			k = 1;
		} else {
			/* bn == 3 or 5 mod 8, expect -1 == bn-1 */
			if (bnAddQ(a, 1) < 0)
				return -1;
			if (bnCmp(a, bn) != 0)
				return 1;	/* Not prime */
			k = 1;
			if (l & 4) {
				/* bn == 5 mod 8, make odd for strong tests */
				bnRShift(e, 1);
				k = 2;
			}
		}
	} else {
		/* bn == 1 mod 8, expect 2^((bn-1)/4) == +/-1 mod bn */
		bnRShift(e, 2);
		if (bnTwoExpMod(a, e, bn) < 0)
			return -1;
		if (bnBits(a) == 1) {
			j = 0;	/* Re-do strong prime test to base 2 */
		} else {
			if (bnAddQ(a, 1) < 0)
				return -1;
			if (bnCmp(a, bn) != 0)
				return 1;	/* Not prime */
		}
		k = 2 + bnMakeOdd(e);
	}
	/* It's prime!  Now go on to confirmation tests */

	/*
	 * Now, e = (bn-1)/2^k is odd.  k >= 1, and has a given value
	 * with probability 2^-k, so its expected value is 2.
	 * j = 1 in the usual case when the previous test was as good as
	 * a strong prime test, but 1/8 of the time, j = 0 because
	 * the strong prime test to the base 2 needs to be re-done.
	 */
	for (i = j; i < sizeof(primes)/sizeof(*primes); i++) {
		if (f && (err = f(arg, '*')) < 0)
			return err;
		(void)bnSetQ(a, primes[i]);
		if (bnExpMod(a, a, e, bn) < 0)
			return -1;
		if (bnBits(a) == 1)
			continue;	/* Passed this test */

		l = k;
		for (;;) {
			if (bnAddQ(a, 1) < 0)
				return -1;
			if (bnCmp(a, bn) == 0)	/* Was result bn-1? */
				break;	/* Prime */
			if (!--l)	/* Reached end, not -1? luck? */
				return i+2-j;	/* Failed, not prime */
			/* This portion is executed, on average, once. */
			(void)bnSubQ(a, 1);	/* Put a back where it was. */
			if (bnSquare(a, a) < 0 || bnMod(a, a, bn) < 0)
				return -1;
			if (bnBits(a) == 1)
				return i+2-j;	/* Failed, not prime */
		}
		/* It worked (to the base primes[i]) */
	}
	
	/* Yes, we've decided that it's prime. */
	if (f && (err = f(arg, '*')) < 0)
		return err;

	return 0;	/* Prime! */
}

/*
 * Modifies the given bnq to return a prime slightly larger, and then
 * modifies the given bnp to be a prime which is == 1 (mod bnq).
 * This is done by decreasing bnp until it is == 1 (mod 2*bnq), and
 * then searching forward in steps of 2*bnq.
 * Returns >=0 on success or -1 on failure (out of memory).  On
 * success, the return value is the number of modular exponentiations
 * performed (excluding the final confirmation).
 * This never gives up searching.
 *
 * int (*f)(void *arg, int c), void *arg
 * The function f argument, if non-NULL, is called with progress indicator
 * characters for printing.  A dot (.) is written every time a primality test
 * is failed, a star (*) every time one is passed, and a slash (/) in the
 * case that the sieve was emptied without finding a prime and is being
 * refilled.  f is also passed the void *arg argument for private
 * context storage.  If f returns < 0, the test aborts and returns
 * that value immediately.
 *
 * Apologies to structured programmers for all the GOTOs.
 */
int
dsaPrimeGen(BigNum *bnq, BigNum *bnp,
	int (*f)(void *arg, int c), void *arg)
{
	int retval;
	unsigned p, prev;
	BigNum a, e;
	int modexps = 0;
#ifdef MSDOS
	unsigned char *sieve;
#else
	unsigned char sieve[SIEVE];
#endif

#ifdef MSDOS
	sieve = bniMemAlloc(SIEVE);
	if (!sieve)
		return -1;
#endif

	bnBegin(&a);
	bnBegin(&e);

	/* Phase 1: Search forwards from bnq for a suitable prime. */

	/* First, make sure that bnq is odd. */
	(void)bnAddQ(bnq, ~bnLSWord(bnq) & 1);

	for (;;) {
		if (sieveBuild(sieve, SIEVE, bnq, 2, 1) < 0)
			goto failed;

		p = prev = 0;
		if (sieve[0] & 1 || (p = sieveSearch(sieve, SIEVE, p)) != 0) {
			do {
				/*
				 * Adjust bn to have the right value,
				 * incrementing in steps of < 65536.
				 * 32767 = 65535/2.
				 */
				pgpAssert(p >= prev);
				prev = p-prev;	/* Delta - add 2*prev to bn */
#if SIEVE*8*2 >= 65536
				while (prev > 32767) {
					if (bnAddQ(bnq, 2*32767) < 0)
						goto failed;
					prev -= 32767;
				}
#endif
				if (bnAddQ(bnq, 2*prev) < 0)
					goto failed;
				prev = p;

				retval = primeTest(bnq, &e, &a, f, arg);
				if (retval <= 0)
					goto phase2;	/* Success! */
				modexps += retval;
				if (f && (retval = f(arg, '.')) < 0)
					goto done;

				/* And try again */
				p = sieveSearch(sieve, SIEVE, p);
			} while (p);
		}

		/* Ran out of sieve space - increase bn and keep trying. */
#if SIEVE*8*2 >= 65536
		p = ((SIEVE-1)*8+7) - prev;	/* Number of steps (of 2) */
		while (p >= 32737) {
			if (bnAddQ(bnq, 2*32767) < 0)
				goto failed;
			p -= 32767;
		}
		if (bnAddQ(bnq, 2*(p+1)) < 0)
			goto failed;
#else
		if (bnAddQ(bnq, SIEVE*8*2 - prev) < 0)
			goto failed;
#endif
		if (f && (retval = f(arg, '/')) < 0)
			goto done;
	} /* for (;;) */

	/*
	 * Phase 2: find a suitable prime bnp == 1 (mod bnq).
	 */

	/*
	 * Since bnp will be, and bnq is, odd, bnp-1 must be a multiple
	 * of 2*bnq.  So start by subtracting the excess.
	 */

phase2:
	/* Double bnq until end of bnp search. */
	if (bnAdd(bnq, bnq) < 0)
		goto failed;

	bnMod(&a, bnp, bnq);
	if (bnBits(&a)) {	/* Will always be true, but... */
		(void)bnSubQ(&a, 1);
		if (bnSub(bnp, &a)) 	/* Also error on underflow */
			goto failed;
	}

	/* Okay, now we're ready. */

	for (;;) {
		if (sieveBuildBig(sieve, SIEVE, bnp, bnq, 0) < 0)
			goto failed;
		if (f && (retval = f(arg, '/')) < 0)
			goto done;

		p = prev = 0;
		if (sieve[0] & 1 || (p = sieveSearch(sieve, SIEVE, p)) != 0) {
			do {
				/*
				 * Adjust bn to have the right value,
				 * adding (p-prev) * 2*bnq.
				 */
				pgpAssert(p >= prev);
				/* Compute delta into a */
				if (bnMulQ(&a, bnq, p-prev) < 0)
					goto failed;
				if (bnAdd(bnp, &a) < 0)
					goto failed;
				prev = p;

				retval = primeTest(bnp, &e, &a, f, arg);
				if (retval <= 0)
					goto done;	/* Success! */
				modexps += retval;
				if (f && (retval = f(arg, '.')) < 0)
					goto done;

				/* And try again */
				p = sieveSearch(sieve, SIEVE, p);
			} while (p);
		}

		/* Ran out of sieve space - increase bn and keep trying. */
#if SIEVE*8 == 65536
		if (prev) {
			p = (unsigned)(SIEVE*8ul - prev);
		} else {
			/* Corner case that will never actually happen */
			if (bnAdd(bnp, bnq) < 0)
				goto failed;
			p = 65535;
		}
#else
		p = SIEVE*8 - prev;
#endif
		/* Add p * bnq to bnp */
		if (bnMulQ(&a, bnq, p) < 0)
			goto failed;
		if (bnAdd(bnp, &a) < 0)
			goto failed;
	} /* for (;;) */

failed:
	retval = -1;

done:
	/* Shift bnq back down by the extra bit again. */
	bnRShift(bnq, 1);	/* Harmless even if bnq is random */

	bnEnd(&e);
	bnEnd(&a);
#ifdef MSDOS
	bniMemFree(sieve, SIEVE);
#else
	bniMemWipe(sieve, sizeof(sieve));
#endif
	return retval < 0 ? retval : modexps + 2*CONFIRMTESTS;
}
