#include "include.h"
using namespace emp;
using namespace std;

//Initialize random rA, rB
inline void dorArB(PRG prg, BT* r_A, BT* r_B) {
  for (size_t i = 0; i < BETA; i++) {
    prg.random_data(&(r_A[i * BUCKETS]), BUCKETS * sizeof(BT));
    prg.random_data(&(r_B[i * BUCKETS]), BUCKETS * sizeof(BT));
  }

  for (size_t i = 0; i < BUCKETS; i++) {
    for (uint64_t j = 0; j < BETA; j++) {
      r_B[i * BETA + j] = r_B[i * BETA + j] % MODULUS;
      r_A[i * BETA + j] = r_A[i * BETA + j] % MODULUS;

      //rBs must have multiplicative inverses
      while (r_B[i * BETA + j] == 0) {
        prg.random_data(&(r_B[i * BETA + j]), sizeof(BT));
        r_B[i * BETA + j] = r_B[i * BETA + j] % MODULUS;
      }

    }
  }
}


void computeTriples(BT* r_A, BT* s_A, BT* r_B, BT* s_B) {
  //Use fixed seed for debugging
  PRG prg(fix_key);

  //Initialize sAs
  prg.random_data(s_A, BUCKETS * sizeof(BT));

  //Init rA, rB
  dorArB(prg, r_A, r_B);

  for (size_t i = 0; i < BUCKETS; i++) {
    //Init sA
    s_A[i] = s_A[i] % MODULUS;
  }

  //Compute sB
  #pragma omp parallel for
  for (uint64_t i = 0; i < BUCKETS; i++) {
    for (uint64_t j = 0; j < BETA; j++) {
      s_B[i * BETA + j] = ((r_A[i * BETA + j] * r_B[i * BETA + j]) - s_A[i] + MODULUS) %  MODULUS;
    }
  }

}

inline void bobsOperations(BT* bobTable, BT* fromAlice, BT* r_B, BT* s_B, BT* INV) {
  //We compute result = ((c + h) + sB) / rB

  #pragma omp parallel for
  for (size_t i = 0; i < BUCKETS; i++) {
    for (size_t j = 0; j < BETA; j++) {
      bobTable[i * BETA + j] = ((fromAlice[i] + bobTable[i * BETA + j] + s_B[i * BETA + j]) * INV[r_B[i * BETA + j]] ) % MODULUS;
    }
  }

}
