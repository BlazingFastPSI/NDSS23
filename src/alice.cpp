//This realizes the paper's online phase
#include "ttp.h"
#include "hash.h"
#include "inv.h"

#include "PackedArray.h"
#include <omp.h>

using namespace emp;
using namespace std;
int port, party;

int main(int argc, char** argv) {
  omp_set_num_threads(THREADS);
  
    if (argc!=3) {
        cout <<"Must supply party and port number"<<endl;
    }

    parse_party_and_port(argv, &party, &port);

    cout <<"Buckets: "<<BUCKETS<<", BETA: "<<BETA<<", #Hash elements: "<<HASHELEMENTS<<", modulus: "<<MODULUS<<endl;
    
    
    //Only known to Alice
    BT *sA = (BT *) calloc(1, BUCKETS*sizeof(BT));
    BT *rA = (BT *) calloc(1, BETA*BUCKETS*sizeof(BT));
    PackedArray* toBobPacked = NULL;
    BT *aliceCuckooTable = NULL;
    BT *toBob = NULL;
    BT *fromBob = NULL;
    PackedArray* fromBobPacked = NULL;

    //Only known to Bob
    BT *INV = (BT*) calloc(1, MODULUS*sizeof(BT));
    BT *bobTable = NULL;
    BT *rB = (BT *) calloc(1, BETA*BUCKETS*sizeof(BT));
    BT *sB = (BT *) calloc(1, BETA*BUCKETS*sizeof(BT));
    PackedArray* fromAlicePacked = NULL;
    BT *fromAlice = NULL;
    
    PackedArray* toAlicePacked = NULL;

    auto quad = clock_start();
    if (party == BOB) {
      computeInverses(INV);
      cout <<"Computing inverses in "<<time_from(quad)/1000<<" ms"<<endl;
    }
    //For simplicity, we quickly generate the same triples on the fly on both parties. This is typically done offline before.
    cout <<"Computing triples "<<flush;
    quad = clock_start();
    computeTriples(rA, sA, rB, sB);
    cout <<"in "<<time_from(quad)/1000<<" ms"<<endl;

    if (party == ALICE) {
      free(rB);
      free(sB);
      free(INV);
    } else {
      free(rA);
      free(sA);
      }
       
    //Fix seed for debugging of fixed input
    block seed = makeBlock(7, 8);
    PRG prg;
    prg.reseed(&seed);

    //We use Cuckoo hashing with 3 hashfunctions. We can prepare these offline.
    BT *h1 = (BT *) calloc(1, HASHELEMENTS*sizeof(BT));
    BT *h2 = (BT *) calloc(1, HASHELEMENTS*sizeof(BT));
    BT *h3 = (BT *) calloc(1, HASHELEMENTS*sizeof(BT));
    
    cout <<"Preparing hash functions..."<<endl;
    auto hashFTime = clock_start();
    computeHashFunctions(prg, h1, h2, h3);
    cout <<"Time to prepare hash functions: "<<time_from(hashFTime)/1000<<" ms"<<endl;
    
    double hashing = 0.0;
    double CPUTime = 0.0;
    
    if(party == ALICE) {
      //Now we start preparing the Cuckoo table
        toBobPacked = PackedArray_create(BITS, BUCKETS);
        aliceCuckooTable = (BT *) calloc(1, BUCKETS*sizeof(BT));
        toBob = (BT *) calloc(1, BUCKETS*sizeof(BT));
        fromBob = (BT *) calloc(1, BETA*BUCKETS*sizeof(BT));
        fromBobPacked = PackedArray_create(BITS, BETA*BUCKETS);

        BT *aliceInput  = (BT*) calloc(1, NELEMENTS * sizeof(BT));
        //Fill Alice with fixed input
	for (size_t i = 0; i<NELEMENTS; i++) {
	  aliceInput[i] = i;
	}
	
        cuckooEntry **bufAliceCuckooTable = (cuckooEntry **) calloc(1, BUCKETS*sizeof(cuckooEntry*));

        auto start = clock_start();

	//Populate Cuckoo table...
        computeCuckoo(aliceInput, h1, h2, h3, bufAliceCuckooTable);

        //...and convert it into a proper format to send it
        for (size_t i = 0; i < BUCKETS; i++) {
            if (bufAliceCuckooTable[i]!=NULL) {
                aliceCuckooTable[i] = (bufAliceCuckooTable[i])->value;
            } else {
	      //Mark as invalid
                aliceCuckooTable[i] = 3*HASHELEMENTS;
            }
        }

	hashing = time_from(start);
	CPUTime += hashing;
        cout <<"Alice Cuckoo hashing time: "<<hashing/1000<<" ms"<<endl;

	//No use for hash functions anymore
	free(h1);
	free(h2);
	free(h3);

	free(bufAliceCuckooTable);
	
        //Compute $c$s for Bob
	double elapsed = 0.0;
        start=clock_start();
        for (size_t i = 0; i < BUCKETS; i++) {
            toBob[i] = (sA[i] - aliceCuckooTable[i] + MODULUS) % MODULUS;
        }
        PackedArray_pack(toBobPacked, 0, toBob, BUCKETS);
	elapsed = time_from(start);
	CPUTime += elapsed;
        cout <<"Alice sA - H(x) time: "<<elapsed/(1000)<<" ms"<<endl;

	free(aliceCuckooTable);
	free(aliceInput);
	
        //END ALICE
    } else {
        //BOB

      //We prepare a regular hash table
        bobTable = (BT *) calloc(1, BETA*BUCKETS*sizeof(BT));
        fromAlicePacked = PackedArray_create(BITS, BUCKETS);
        fromAlice = (BT *) calloc(1, BUCKETS*sizeof(BT));
        toAlicePacked = PackedArray_create(BITS, BETA*BUCKETS);
	
        //Compute fixed input
        BT *bobInput  = (BT*) malloc(NELEMENTS * sizeof(BT));
        for (size_t i = 0; i<NELEMENTS; i++) {
	  bobInput[i] = i;
	}
	
        bucket *_bobTable = (bucket*) calloc(1, BUCKETS*sizeof(bucket));

        auto t = clock_start();

	//Compute regular hash table
	computeHash(bobInput, h1, h2, h3, _bobTable, bobTable);
	
	hashing = time_from(t);
	CPUTime += hashing;
        cout <<"Bob hashing: "<<hashing/1000<<" ms"<<endl;

	//No use for hash functions anymore
	free(h1);
	free(h2);
	free(h3);

	free(_bobTable);
	free(bobInput);
        //END BOB
    }

    cout <<"Connecting..."<<endl;
    double total_time = 0.0;
    auto ttime_including_idle = clock_start();

    //Server or local environment: NetIO* io = new NetIO(party==ALICE?nullptr:"127.0.0.1", port);
    //West Coast: NetIO* io = new NetIO(party==ALICE?nullptr:"104.42.191.30", port);
    //EU: NetIO* io = new NetIO(party==ALICE?nullptr:"13.95.162.209", port);

    
    NetIO* io = new NetIO(party==ALICE?nullptr:"127.0.0.1", port);
    
    if (party == ALICE) {
      //Send the $c$s
      
        //4 = number of bytes per uint32_t slot
        size_t myLength = 4*PackedArray_bufferSize(toBobPacked);
        cout <<"Alice sending "<<myLength<<" Bytes"<<endl;
        io->sync();
	
	ttime_including_idle = clock_start();
        auto start=clock_start();
        mySend(io, (char*)toBobPacked, myLength);
        auto elapsed = time_from(start);
        total_time +=elapsed;
        cout <<"Alice sent in "<<elapsed/1000<<" ms"<<endl;

	free(toBobPacked);

	//Alice gets back $d$s
        myLength = 4*PackedArray_bufferSize(fromBobPacked);
        cout <<"Alice receiving "<<myLength<<" Bytes"<<endl;
        io->sync();
        start=clock_start();

        myRecv(io, (char*) fromBobPacked, myLength);

	auto pack = clock_start();
        PackedArray_unpack(fromBobPacked, 0, fromBob, BETA * BUCKETS);
	CPUTime += time_from(pack);
	
        elapsed = time_from(start);
        total_time +=elapsed;
        cout <<"Alice received in "<<elapsed/1000<<" ms"<<endl;
	free(fromBobPacked);
 
        //Compute intersection
        start=clock_start();
        size_t match = 0;
	#pragma omp parallel for reduction(+:match)
        for (size_t i = 0; i < BUCKETS; i++) {
	  for (size_t j = 0; j < BETA; j++) {
                if (rA[i*BETA + j]==fromBob[i * BETA + j]) {
                    match++;
		                    }
            }
        }
        elapsed = time_from(start);
        total_time +=elapsed;
	CPUTime+=elapsed;
        cout <<"Alice computes intersection in "<<elapsed/1000<<" ms, size "<<match<<endl;
	
	//END ALICE
    } else {
        //BOB

      //Bob gets the $c$s
      
        //4 = number of bytes per uint32_t slot
        size_t myLength = 4*PackedArray_bufferSize(fromAlicePacked);
        //BOB
        cout <<"Bob receiving "<<myLength<<" Bytes"<<endl;
        io->sync();
	
	ttime_including_idle = clock_start();
        auto start=clock_start();

        //io->recv_data(fromAlicePacked, myLength);
	myRecv(io, (char *) fromAlicePacked, myLength);

	auto unpack = clock_start();
        PackedArray_unpack(fromAlicePacked, 0, fromAlice, BUCKETS);
	auto elapsed = time_from(start);
	CPUTime +=time_from(unpack);
        total_time +=elapsed;
        cout <<"Bob received in "<<elapsed/1000<<" ms"<<endl;

	free(fromAlicePacked);
        
        //Compute result: d =  (fromAlice + myHash + sB) / rB 
        start = clock_start();
	bobsOperations(bobTable, fromAlice, rB, sB, INV);
	PackedArray_pack(toAlicePacked, 0, bobTable, BETA*BUCKETS);
	
        elapsed = time_from(start);
        total_time +=elapsed;
	CPUTime+=elapsed;
        cout <<"Bob computes answer in "<<elapsed/1000<<" ms"<<endl;

	free(fromAlice);
	free(bobTable);

	//Send all $d$s back
        myLength = 4*PackedArray_bufferSize(toAlicePacked);
        cout <<"Bob sending "<<myLength<<" Bytes"<<endl;
        io->sync();
        start=clock_start();

	mySend(io, (char*) toAlicePacked, myLength);
	
        elapsed = time_from(start);
        total_time +=elapsed;
        cout <<"Bob sent in "<<elapsed/1000<<" ms"<<endl;

	free(toAlicePacked);
    }

    cout <<"Total time for party "<<myName<<": "<<total_time/1000<<" ms, sent: "<<(io->counter)<<" Byte, i.e., "<<((double)io->counter)/(1024*1024)<<" MByte"<<endl;

    cout <<"Total time including waiting for "<<myName<<": "<<time_from(ttime_including_idle)/1000<<" ms"<<endl;

    cout <<"CPU time for "<<myName<<": "<<(CPUTime)/1000<<" ms"<<endl;

    
    cout <<"Total time including waiting and hashing for "<<myName<<": "<<(time_from(ttime_including_idle)+hashing)/1000<<" ms"<<endl;

    
}
