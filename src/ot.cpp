#include "ttp.h"
#include "../emp-ot/test/test.h"
#include "ot.h"

#include <omp.h>

using namespace std;

//This is the OT offline phase

int main(int argc, char** argv) {
  omp_set_num_threads(THREADS);
  int threads = omp_get_num_procs(); //THREADS

  if (argc != 3) {
    cout << "Must supply party and port number" << endl;
  }

  int port, party;
  parse_party_and_port(argv, &party, &port);

  //Fixed key for debugging
  PRG prg(fix_key);
  
  BT* rA = (BT*) calloc(1, BETA * BUCKETS * sizeof(BT));
  BT* rB = (BT*) calloc(1, BETA * BUCKETS * sizeof(BT));
  cout << "Number of triples: " << BETA* BUCKETS << endl;
  cout << "Computing rAs, rBs " << flush;

  auto myTime = clock_start();

  //Obviously each party would only compute their own rA or rB
  dorArB(prg, rA, rB);
  double elapsed = time_from(myTime);
  cout << "in " << elapsed / 1000 << " ms" << endl;
 
  if (party == ALICE) {
    cout << "Number of OTs will we perform: " << ((size_t)BUCKETS)* BETA* BITS << endl;
    free(rB);
  } else {//BOB
    free(rA);
  }

  cout << "Connecting..." << flush;
  //IKNP
  //NetIO* io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);
  //FERRET
  NetIO* ios[threads];

  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);

  myTime = clock_start();

  //#define OT FerretCOT
  #define OT IKNP


  //Ferret
  //FerretCOT<NetIO>* ot = new FerretCOT<NetIO>(party, threads, ios, false);
  
  //IKNP
  IKNP<NetIO> * ot = new IKNP<NetIO>(ios[0]);

  setup_ot<OT<NetIO>>(ot, ios[0], party);
  elapsed += time_from(myTime);
  
  block* r = NULL;
  bool* b = NULL;
  BT* sB = NULL;
  BT* sA = NULL;

  if (party == ALICE) {
    myTime = clock_start();
    
    //Compute S, some random values
    //Alice's share sA will be the sum of all random values in S
    //Bob's share will remove random values from S and add rB[i] * rA * 2^i
    //We will do OT, where Bob chooses bit rB[i] = 0 or 1 and will get either the negative random value from S or the negative random values from S plus rA*2^i

    sA = (BT*) calloc(1, BUCKETS * sizeof(BT));

    if (LOGN > 20) {
      for (size_t i = 0; i < (1 << (LOGN - 20)); i++) {
        cout << "i: " << i << endl;
        computeShares<OT<NetIO>>(ot, ios[0], &(sA[i * (1 << 20)]), &(rA[i * (1 << 20)*BETA]), party, NULL, NULL, TWO_TO_TWENTY_BUCKETS);
      }
    } else {
      computeShares<OT<NetIO>>(ot, ios[0], sA, rA, party, NULL, NULL, BUCKETS);
    }

    elapsed += time_from(myTime);
  } else {//BOB
    myTime = clock_start();
    
    sB = (BT*) calloc(1, BUCKETS * BETA * sizeof(BT));

    //For larger values, we need to run a loop to save RAM
    if (LOGN > 20) {
      for (size_t i = 0; i < (1 << (LOGN - 20)); i++) {
        computeShares<OT<NetIO>>(ot, ios[0], NULL, NULL, party, &(rB[i * (1 << 20)*BETA]), &(sB[i * (1 << 20)*BETA]), TWO_TO_TWENTY_BUCKETS);
      }
    } else {
      computeShares<OT<NetIO>>(ot, ios[0], NULL, NULL, party, rB, sB, BUCKETS);
    }

    elapsed += time_from(myTime);
  }
  
  if (party == ALICE) {
    free(sA);
  } else {//BOB
    delete[] r;
    delete[] b;
    free(sB);
  }

  cout << myName << " total time: " << elapsed / 1000 << " ms. Time per triple: " << elapsed / (BETA * BUCKETS) << " us."<< endl;
  cout << myName << " data sent: " << ((double)ios[0]->counter) / (1024 * 1024) << " MByte" << endl;
  delete ot;
}

