UNAME_S := $(shell uname -s)
#-ftree-vectorize -fopt-info-vec-missed
#-O6
CC=g++-10
CFLAGS =-O6 -fopenmp -Wall -I/usr/local/include -I../emp-tool -I../emp-ot  -I/usr/local/opt/openssl/include -march=native -maes -mrdseed -Wno-ignored-attributes  -I/usr/local/opt/gmp/include  -std=c++17 -I../PackedArray
LDFLAGS =-fopenmp -L/usr/local/lib -L../emp-tool -L/usr/local/opt/openssl/lib -L/usr/local/opt/gmp/lib 

ifeq ($(UNAME_S),Darwin)
    CC=g++
    CFLAGS =-Xpreprocessor -fopenmp -O3 -Wall -I/usr/local/include -I../emp-tool -I../emp-ot  -I/usr/local/opt/openssl/include -march=native -maes -mrdseed -Wno-ignored-attributes  -I/usr/local/opt/gmp/include -std=c++17 -I../PackedArray
    LDFLAGS =-L/usr/local/lib -L../emp-tool -L/usr/local/opt/openssl/lib -L/usr/local/opt/gmp/lib/ -lomp

endif
.DEFAULT_GOAL := all

DEPS = include.h ttp.h include-20.h include-22.h include-24.h include-26.h hash.h inv.h ot.h 

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

PackedArraySIMD.o: ../PackedArray/PackedArraySIMD.c
	$(CC) -O3 -DNDEBUG -c ../PackedArray/PackedArraySIMD.c 

opt-sender: opt-sender.o PackedArraySIMD.o 
	$(CC) $(LDFLAGS) -o opt-sender opt-sender.o PackedArraySIMD.o 
	cp opt-sender opt-receiver

ot: ot.o
	$(CC) $(LDFLAGS) -o ot ot.o -lcrypto 

alice: alice.o PackedArraySIMD.o
	$(CC) $(LDFLAGS) -o alice alice.o PackedArraySIMD.o  
	cp alice bob

all: ot alice opt-sender 

clean:
	rm -f *.o ot alice bob opt-sender opt-receiver 
