//This is the version where only the seed is exchanged
#include "ttp.h"
#include "PackedArray.h"
#include <omp.h>

using namespace emp;
using namespace std;
int port, party;


int main(int argc, char** argv) {

  if (argc != 3) {
    cout << "Must supply party and port number" << endl;
  }

  parse_party_and_port(argv, &party, &port);
  omp_set_num_threads(THREADS);
  
  //Use fixed key for debugging
  PRG prg(fix_key);

  auto start = clock_start();

  if (party == ALICE) {

    cout << "Number of buckets: " << BUCKETS << endl;
    cout << "Number of comparisons/bucket: " << BETA << endl;
    cout << "Size of basetype: " << sizeof(BT) << " Byte" << endl;

    BT* s_A = (BT*) calloc(1, BUCKETS * sizeof(BT));
    BT* r_A = (BT*) calloc(1, BETA * BUCKETS * sizeof(BT));

    BT* r_B = (BT*) calloc(1, BETA * BUCKETS * sizeof(BT));
    BT* s_B = (BT*) calloc(1, BETA * BUCKETS * sizeof(BT));

    cout << "Number of sAs: " << BUCKETS << endl;
    cout << "Number of rAs, rBs, sBs: " << BUCKETS* BETA << endl;

    start = clock_start();
    computeTriples(r_A, s_A, r_B, s_B);
    auto elapsed = time_from(start);
    double ttpCPU = elapsed;
    cout << "Total TTP time to generate all triples: " << elapsed / (1000) << " ms" << endl;
    cout << "Time per quadruple: " << 1000UL * elapsed / (1UL * BUCKETS * BETA) << " ns" << endl;
    cout << "Time per bucket: " << 1000UL * elapsed / (1UL * BUCKETS) << " ns" << endl;

    //Note that we are actually not sending a sending the 16 Byte seed to Alice and to Bob...this is for free.

    NetIO* io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);

    start = clock_start();

    //At this point, we are the TTP. 
    //We send rAs to Alice
    PackedArray* rAPacked = NULL;
    rAPacked = PackedArray_create(BITS, BUCKETS * BETA);
    PackedArray_pack(rAPacked, 0, r_A, BUCKETS * BETA);
    size_t rALength = 4 * PackedArray_bufferSize(rAPacked);
    ttpCPU += time_from(start);

    cout << "Sending " << ((double)rALength)/(1024*1024) << " MByte to Alice..." << endl;
    io->sync();
    start = clock_start();

    //This goes to Alice
    io->send_data(&rALength, sizeof(size_t) );
    mySend(io, (char*)rAPacked, rALength);

    double commTime = time_from(start);
    cout << "TTP comm. time: " << commTime / (1000UL) << " ms" << endl;
    cout << "Volume sent: " << (double)io->counter / (1024 * 1024) << " MByte" << endl;
    cout << "TTP CPU time: " << ttpCPU / 1000 << " ms" << endl;
    double ttpTotalTime = (commTime + ttpCPU);
    cout << "TTP total time: " << ttpTotalTime / 1000 << " ms" << endl;

    //Now, we become Bob.
    //Bob has to compute rB and sB
    start = clock_start();
    
    for (size_t i = 0; i < BETA; i++) {
      prg.random_data(&(r_B[i * BUCKETS]), BUCKETS * sizeof(BT));
      prg.random_data(&(s_B[i * BUCKETS]), BUCKETS * sizeof(BT));
    }

    //Init rB, sB
    for (size_t i = 0; i < BUCKETS; i++) {
      for (uint64_t j = 0; j < BETA; j++) {
        s_B[i * BETA + j] = s_B[i * BETA + j] % MODULUS;
        while (r_B[i * BETA + j] == 0) {
          prg.random_data(&(r_B[i * BETA + j]), sizeof(BT));
          r_B[i * BETA + j] = r_B[i * BETA + j] % MODULUS;
        }
      }
    }

    double cpuTime = time_from(start);
    cout << "Bob CPU time: " << cpuTime / 1000 << " ms" << endl;
    cout << "Bob total time: " << cpuTime / 1000 << " ms" << endl;
    mySend(io, (char*) &ttpTotalTime, sizeof(double));
    mySend(io, (char*) &ttpCPU, sizeof(double));

  } else { //Alice
    NetIO* io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);

    //Alice receives the rAs from the TTP
    cout << "Receiving from TTP..." << endl;

    size_t totalComm;
    io->sync();
    io->recv_data(&totalComm, sizeof(size_t));
    char* buf = (char*) malloc(totalComm);

    if (buf == NULL) {
      cout << "No memory left" << endl;
    }

    start = clock_start();
    myRecv(io, buf, totalComm);
    double commTime = time_from(start);
    cout << "Comm. time Alice:"<< commTime / (1000UL) << " ms" << endl;

    //Now Alice has to compute sA
    BT* s_A = (BT*) calloc(1, BUCKETS * sizeof(BT));
    start = clock_start();

    prg.random_data(s_A, BUCKETS * sizeof(BT));
    

    for (size_t i = 0; i < BUCKETS; i++) {
      s_A[i] = s_A[i] % MODULUS;
    }

    double cpuTime = time_from(start);
    cout << "Alice CPU Time: " << cpuTime / 1000 << " ms" << endl;
    cout << "Alice Total Time: " << (cpuTime + commTime) / 1000 << " ms" << endl;
    double ttpTotalTime;
    myRecv(io, (char*) &ttpTotalTime, sizeof(double));
    double ttpCPU;
    myRecv(io, (char*) &ttpCPU, sizeof(double));
    cout << "---CPU Time: " << (ttpCPU + cpuTime) / 1000 << " ms" << endl;
    cout << "---Total Offline Time---: " << (cpuTime + ttpTotalTime) / 1000 << " ms" << endl;

  }

}
