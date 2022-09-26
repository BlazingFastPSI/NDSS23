#include "Enclave.h"
#include "Enclave_t.h" /* print_string */
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>

#include <sgx_tcrypto.h>

#include "prg.h"

//For n = 2^26
//#define BT uint16_t
//For n < 2 ^26
#define BT uint32_t
#define ALICE 1
#define BOB 2

#define store128(x) (_mm_loadu_si128((__m128i_u*)(x)))

using namespace emp;

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char* fmt, ...) {
  char buf[BUFSIZ] = { '\0' };
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_print_string(buf);
  return (int)strnlen(buf, BUFSIZ - 1) + 1;
}


inline void dorArB(PRG prg, BT* r_A, BT* r_B, size_t BUCKETS, size_t BETA, BT MODULUS) {
  //Init rA, rB

  for (size_t i = 0; i < BETA; i++) {
    prg.random_data(&(r_A[i * BUCKETS]), BUCKETS * sizeof(BT));
    prg.random_data(&(r_B[i * BUCKETS]), BUCKETS * sizeof(BT));
  }

  for (size_t i = 0; i < BUCKETS; i++) {
    for (uint64_t j = 0; j < BETA; j++) {
      r_B[i * BETA + j] = r_B[i * BETA + j] % MODULUS;
      r_A[i * BETA + j] = r_A[i * BETA + j] % MODULUS;

      while (r_B[i * BETA + j] == 0) {
        prg.random_data(&(r_B[i * BETA + j]), sizeof(BT));
        r_B[i * BETA + j] = r_B[i * BETA + j] % MODULUS;
      }

    }
  }
}


void computeQuadruples(BT* r_A, BT* s_A, BT* r_B, BT* s_B, size_t BUCKETS, size_t BETA, BT MODULUS) {
  //Use fixed seed for debugging
  PRG prg(fix_key);

  prg.random_data(s_A, BUCKETS * sizeof(BT));

  //Init rA, rB
  dorArB(prg, r_A, r_B, BUCKETS, BETA, MODULUS);

  for (size_t i = 0; i < BUCKETS; i++) {
    //Init sA
    s_A[i] = s_A[i] % MODULUS;
  }

  //Compute sB
  for (uint64_t i = 0; i < BUCKETS; i++) {
    for (uint64_t j = 0; j < BETA; j++) {
      s_B[i * BETA + j] = ((r_A[i * BETA + j] * r_B[i * BETA + j]) - s_A[i] + MODULUS) %  MODULUS;
    }
  }


}


void SGXBench(size_t BUCKETS, size_t BETA, int party, BT* r, BT* s, BT MODULUS) {

  BT* rA, *sA, *rB, *sB;

  if (party == ALICE) {
    rA = r;
    sA = s;
    rB = (BT*) calloc(1, BUCKETS * BETA * sizeof(BT));
    sB = (BT*) calloc(1, BUCKETS * BETA * sizeof(BT));
  } else {//BOB
    rB = r;
    sB = s;
    rA = (BT*) calloc(1, BUCKETS * BETA * sizeof(BT));
    sA = (BT*) calloc(1, BUCKETS * sizeof(BT));
  }

  if ((rB == NULL) || (sB == NULL) || (rA == NULL) || (sA == NULL)) {
    printf("NULL --- increase heap\n");
    return;
  }

  computeQuadruples(rA, sA, rB, sB, BUCKETS, BETA, MODULUS);

  if (party == ALICE) {
    free(rB);
    free(sB);
  } else {//BOB
    free(rA);
    free(sA);
  }


}

void SGXGetPRFs(size_t n, unsigned char* input) {
  //    sgx_status_t SGXAPI sgx_sha256_msg(const uint8_t *p_src, uint32_t src_len, sgx_sha256_hash_t *p_hash);

  for (size_t i = 0; i < n; i++) {
    unsigned char buffer[32];
    sgx_sha256_msg(&input[32 * i], 32, (sgx_sha256_hash_t*) buffer);
    memcpy(&input[32 * i], buffer, 32);
  }

}
