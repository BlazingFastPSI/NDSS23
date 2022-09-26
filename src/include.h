#include "emp-tool/emp-tool.h"
#include <gmp.h>    // gmp is included implicitly

//Statistical security parameter
#define LAMBDA 40


/*Recall that we have to take |xR| * 2 as minimal bitlength as we do
  multiplications with rBs "in the clear"*/
//#define BT uint32_t -- we define this in the sub-include files

/* We are using 3 hash functions.  We operate in GF(p), where p is the
   smallest prime larger than 3 * |xR| + 2.  We encode xR and store
   i * 2^xRLEN + xR into a bucket, where i is the hash function.

   Alice's dummy elements will be 3 * 2^xRLEN, and Bob's will be 3 *
   2^xRLEN + 1.
 */
#include "include-20.h"

#define NELEMENTS (1<<LOGN)

#define TWO_TO_TWENTY_BUCKETS 1331692

//This is 2^xRLEN
#define HASHELEMENTS (1<<X_RLEN)

#define UNIVERSE (1UL<<ELL)

#define THREADS 1
//#define THREADS (omp_get_num_procs())

inline void recv_mpz(emp::NetIO *io, mpz_t data) {
    size_t count;
    io->recv_data(&count, sizeof(count));
    char *buf = (char *) malloc(count);
    //std::cout <<"Receiving "<<count<<" bytes"<<endl;
    io->recv_data(buf, count);
    mpz_import(data, count, 1, 1, 0, 0, buf);
}

inline void send_mpz(emp::NetIO *io, mpz_t data) {
    size_t count = 0;
    char *buf = (char *) mpz_export(NULL, &count, 1, 1, 0, 0, data);
    //std::cout <<"Sending "<<count<<" bytes"<<endl;
    io->send_data(&count, sizeof(count));
    io->send_data(buf, count);
    free(buf);
}

inline void mySend(emp::NetIO* io, char* buf, size_t len) {

  if (len>1<<30) {
    io->send_data(buf, 1<<30);
    mySend(io, &buf[1<<30], len - (1<<30));
  } else {
      io->send_data(buf, len);
  }

}

inline void myRecv(emp::NetIO* io, char* buf, size_t len) {

  if (len>1<<30) {
    io->recv_data(buf, 1<<30);
    myRecv(io, &buf[1<<30], len - (1<<30));
  } else {
    io->recv_data(buf, len);
  }
}

#define myName ((party==ALICE)?"ALICE":"BOB")

inline void printHex(unsigned char* str, int length) {
  for (int tmp = length - 1; tmp >= 0; tmp--) {
    printf("%02x", str[tmp]);
  }
}
