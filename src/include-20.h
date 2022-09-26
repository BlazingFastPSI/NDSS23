#define ELL 32
#define LOGN 20
#define BUCKETS 1331692
#define BT uint32_t
#define BETA 28

//ELL-log(BUCKETS)
#define X_RLEN 12

//This is the number of bits of the modulus, log (MODULUS)
#define BITS 14

//We have to represent eta = (#hash functions) * 2^X_RLEN + 2 different dummy values (one for Alice, one for Bob)
//The modulus is nextPrime(eta)
#define MODULUS 12301

