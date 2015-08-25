#ifdef __cplusplus
extern "C" {
#endif

int
dhFirstHalf(unsigned char const *modulus, unsigned modlen, unsigned explen,
	unsigned char const *generator, unsigned genlen,
	unsigned char *oursecret, unsigned oursecretlen,
	unsigned char *ourpublic);
int
dhSecondHalf(unsigned char const *modulus, unsigned modlen,
	unsigned char const *generator, unsigned genlen,
	unsigned char const *oursecret, unsigned oursecretlen,
	unsigned char const *theirpublic, unsigned theirpubliclen,
	struct ByteFifo *SharedSecretFifo,
	struct ByteFifo *AuthenticationFifo);

extern unsigned char const dhPrime[512/8];
extern unsigned char const dhGenerator[1];

#ifdef __cplusplus
}
#endif

