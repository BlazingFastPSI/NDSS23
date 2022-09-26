//convert base type BT to an EMP block
inline void blockToBT(block* x, BT* y) {
  memcpy(y, x,  sizeof(BT));
}


template <typename T>
void setup_ot(T* ot, NetIO* io, int party) {
  block* b0 = new block[1], *b1 = new block[1],
  *r = new block[1];

  //Fixed key for debugging
  PRG prg(fix_key);
  
  prg.random_block(b0, 1);
  prg.random_block(b1, 1);
  bool* b = new bool[1];
  PRG prg2;
  prg2.random_bool(b, 1);

  if (party == ALICE) {
    ot->send(b0, b1, 1);
  } else {
    ot->recv(r, b, 1);
  }

  io->flush();
  delete[] b0;
  delete[] b1;
  delete[] r;
  delete[] b;
}


//Do an 1-oo-2 OT
template <typename T>
inline void doOT(T* ot, NetIO* io, int party, size_t length, block* b0, block* b1, block* r, bool* b) {

  io->sync();

  if (party == ALICE) {
    ot->send(b0, b1, length);
  } else {
    ot->recv(r, b, length);
  }

  io->flush();
}

//Use Gilboa's multiplication with 1-oo-2 OTs
template <typename T>
void computeShares(T* myCOT, NetIO* io, BT* sA, BT* rA, int party, BT* rB, BT* sB, size_t buckets) {

  if (party == ALICE) {
    PRG prg(fix_key);

    BT* LEFT = NULL;
    BT* RIGHT = NULL;

    LEFT = (BT*) malloc(buckets * BETA * BITS * sizeof(BT));
    RIGHT = (BT*) malloc(buckets * BETA * BITS * sizeof(BT));

    block* b0 = new block[buckets * BITS * BETA];
    block* b1 = new block[buckets * BITS * BETA];

    //Iterate over all buckets
    for (size_t i = 0; i < buckets; i++) {
      //Choose random sA
      prg.random_data(&(sA[i]), sizeof(BT));
      sA[i] = sA[i] % MODULUS;

      for (size_t j = 0; j < BETA; j++) {

        //Compute S values for this bucket
        BT S[BITS];
        prg.random_data(S, BITS * sizeof(BT));

        size_t sum = 0;

        for (size_t j = 0; j < BITS - 1; j++) {
          S[j] = S[j] % MODULUS;
          sum = (sum + S[j]) %  MODULUS;
        }

        S[BITS - 1] = (MODULUS + sum - sA[i]) % MODULUS;

        //Compute left and right values for this bucket
        //Go over all bits
        for (size_t k = 0; k < BITS; k++) {
          size_t index = i * BETA * BITS + j * BITS + k;

          LEFT[index] = MODULUS - S[k];
          RIGHT[index] = (rA[i * BETA + j] * (1 << k)) % MODULUS;
          RIGHT[index] = (MODULUS + RIGHT[index] - S[k]) % MODULUS;

          /*if (j == 10) {
            cout << "L: " << LEFT[index] << ", R: " << RIGHT[index] <<", index: "<<index<< endl;
          }*/

          b0[index] = makeBlock(0, LEFT[index]);
          b1[index] = makeBlock(0, RIGHT[index]);
        }
      }
    }

    cout << "Alice starting OT..." << flush;
    doOT<T>(myCOT, io, party, buckets * BITS * BETA, b0, b1, NULL, NULL);
    cout << "done" << endl;
    free(LEFT);
    free(RIGHT);

    delete[] b0;
    delete[] b1;

  } else { //Bob

    block* r = new block[buckets * BITS * BETA];
    bool* b = new bool[buckets * BITS * BETA];

    for (size_t i = 0; i < buckets; i++) {
      for (size_t j = 0; j < BETA; j++) {
        int_to_bool(&(b[i * BITS * BETA + j * BITS]), rB[i * BETA + j], BITS);
      }
    }

    cout << "Bob starting OT..." << flush;
    doOT<T>(myCOT, io, party, buckets * BITS * BETA, NULL, NULL, r, b);
    cout << "done" << endl;

    delete[] b;

    for (size_t i = 0; i < buckets; i++) {
      for (size_t j = 0; j < BETA; j++) {
        for (size_t k = 0; k < BITS; k++) {
          BT tmp;
          blockToBT(&(r[i * BETA * BITS + j * BITS + k]), &tmp);

          sB[i * BETA + j] = (sB[i * BETA + j] + tmp) % MODULUS;
        }

      }
    }

    delete[] r;
  }
}
