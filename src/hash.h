#include <omp.h>

using namespace std;


//Here we define data structures for and operations on hash tables

typedef struct cuckooEntry {
    BT p[3];
    BT value;
    uint8_t choice;

} cuckooEntry;


typedef struct bucket {
    BT values[BETA];
    uint8_t load;
} bucket;



inline void computeCuckoo(BT *aliceInput, BT *h1, BT *h2, BT *h3, cuckooEntry **_aliceCuckooTable) {

   omp_lock_t *writelock = (omp_lock_t *) malloc(BUCKETS*sizeof(omp_lock_t));
  for (size_t i = 0;i<BUCKETS;i++) {
    omp_init_lock(&(writelock[i]));
    }

  
    uint64_t quotient = UNIVERSE/BUCKETS;

    #pragma omp parallel for
    for (size_t i = 0; i < NELEMENTS; i++) {

      //Permutation-based hashing for arbitrary length hash tables
      BT xr = aliceInput[i] % (quotient);
      BT xl = aliceInput[i] / (quotient);
      
      
        cuckooEntry *e = (cuckooEntry *) malloc(sizeof(cuckooEntry));

      //Permutation-based hashing for arbitrary length hash tables
	//Compute the 3 possible buckets
	e->p[0] = (h1[xr] + xl) % BUCKETS;
        e->p[1] = (h2[xr] + xl) % BUCKETS;
        e->p[2] = (h3[xr] + xl) % BUCKETS;
	
	//This code could be used if our hash tables would have power-of-2-length
	/*e->p[0] = (h1[xr] ^ xl) % BUCKETS;
        e->p[1] = (h2[xr] ^ xl) % BUCKETS;
        e->p[2] = (h3[xr] ^ xl) % BUCKETS;
	*/
	
        e->value = xr;
        e->choice = 0;

	auto index = e->p[e->choice];
	omp_set_lock(&(writelock[index]));
	swap(e, _aliceCuckooTable[e->p[e->choice]]);
	omp_unset_lock(&(writelock[index]));

	//At this point, we are holding one hash table entry e in our hand. We are now going to swap this entry with entries in the cuckoo table until we find an empty entry in the table
	
        while(e!=NULL) {
	  //Swap entries
            e->choice = (e->choice + 1) % 3;

	    index = e->p[e->choice];
	    omp_set_lock(&(writelock[index]));
	    swap(e, _aliceCuckooTable[e->p[e->choice]]);
	    omp_unset_lock(&(writelock[index]));
	    //Now, we are holding a different e in our hand
        }
    }

    //Add number indicating hash function to end of input!
    for (size_t i=0; i<BUCKETS; i++) {
        if(_aliceCuckooTable[i]!=NULL) {
            _aliceCuckooTable[i]->value = (_aliceCuckooTable[i]->value ) +  HASHELEMENTS*_aliceCuckooTable[i]->choice;
        }
    }

}

inline void computeHashFunctions(PRG prg, BT *h1, BT *h2, BT *h3) {

  //Use a simple PRG to get uniform distribution. 
    std::mt19937 gen(0); // seed the generator
    std::uniform_int_distribution<BT> dist(0, BUCKETS-1); // define the range
    
    for (size_t i = 0; i<HASHELEMENTS; i++) {
      h1[i] = dist(gen);
     h2[i] = dist(gen);
     h3[i] = dist(gen);
     while (h2[i] == h1[i]) {
	h2[i] = dist(gen);
     }

     while ((h3[i] == h1[i])||(h3[i] == h2[i])) {
       h3[i] = dist(gen);
     }


    }   
}

inline void computeHash(BT *bobInput, BT *h1, BT *h2, BT *h3, bucket *_bobTable, BT *bobTable) {
          uint64_t quotient = UNIVERSE/BUCKETS;

	  //pragma omp parallel for 
        for (size_t i = 0; i < NELEMENTS; i++) {

	  //Permutation-based hashing
      BT xr = bobInput[i] % (quotient);
      BT xl = bobInput[i] / (quotient);
            auto index1 = (h1[xr] + xl) % BUCKETS;
            auto index2 = (h2[xr] + xl) % BUCKETS;
            auto index3 = (h3[xr] + xl) % BUCKETS;

	    //Put it in all three locations
	    //Mark which hash function we are using
            bucket *b = &_bobTable[index1];
            b->values[b->load] = (xr);
            b->load++;

            b = &_bobTable[index2];
            b->values[b->load] = (HASHELEMENTS+xr); 
            b->load++;

            b = &_bobTable[index3];
            b->values[b->load] = (2*HASHELEMENTS+xr);
            b->load++;

        }

	#pragma omp parallel for
        for (size_t i = 0; i<BUCKETS; i++) {
	  bucket *b =  &_bobTable[i];
            for (size_t j = 0;j<BETA;j++) {
	      if (j<b->load) {
		bobTable[i*BETA+j] = b->values[j]; 
	      } else {
		//Mark as invalid
		bobTable[i*BETA+j] = 3*HASHELEMENTS+1;
	      }
            }
        }


  
}
