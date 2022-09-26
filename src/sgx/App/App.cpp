#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include <sgx_uswitchless.h>
#include "App.h"
#include "Enclave_u.h"

#include "../../include.h"

#include<unordered_map>


using namespace std;
using namespace emp;


#define PRFLENGTH (32)

string hex2String(unsigned char* str, size_t length) {
  std::stringstream ss;

  for (size_t i = 0; i < length; ++i) {
    ss << std::hex << (int)str[i];
  }

  return ss.str();
}

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
  sgx_status_t err;
  const char* msg;
  const char* sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
  {
    SGX_ERROR_UNEXPECTED,
    "Unexpected error occurred.",
    NULL
  },
  {
    SGX_ERROR_INVALID_PARAMETER,
    "Invalid parameter.",
    NULL
  },
  {
    SGX_ERROR_OUT_OF_MEMORY,
    "Out of memory.",
    NULL
  },
  {
    SGX_ERROR_ENCLAVE_LOST,
    "Power transition occurred.",
    "Please refer to the sample \"PowerTransition\" for details."
  },
  {
    SGX_ERROR_INVALID_ENCLAVE,
    "Invalid enclave image.",
    NULL
  },
  {
    SGX_ERROR_INVALID_ENCLAVE_ID,
    "Invalid enclave identification.",
    NULL
  },
  {
    SGX_ERROR_INVALID_SIGNATURE,
    "Invalid enclave signature.",
    NULL
  },
  {
    SGX_ERROR_OUT_OF_EPC,
    "Out of EPC memory.",
    NULL
  },
  {
    SGX_ERROR_NO_DEVICE,
    "Invalid SGX device.",
    "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
  },
  {
    SGX_ERROR_MEMORY_MAP_CONFLICT,
    "Memory map conflicted.",
    NULL
  },
  {
    SGX_ERROR_INVALID_METADATA,
    "Invalid enclave metadata.",
    NULL
  },
  {
    SGX_ERROR_DEVICE_BUSY,
    "SGX device was busy.",
    NULL
  },
  {
    SGX_ERROR_INVALID_VERSION,
    "Enclave version was invalid.",
    NULL
  },
  {
    SGX_ERROR_INVALID_ATTRIBUTE,
    "Enclave was not authorized.",
    NULL
  },
  {
    SGX_ERROR_ENCLAVE_FILE_ACCESS,
    "Can't open enclave file.",
    NULL
  },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret) {
  size_t idx = 0;
  size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

  for (idx = 0; idx < ttl; idx++) {
    if (ret == sgx_errlist[idx].err) {
      if (NULL != sgx_errlist[idx].sug)
        printf("Info: %s\n", sgx_errlist[idx].sug);

      printf("Error: %s\n", sgx_errlist[idx].msg);
      break;
    }
  }

  if (idx == ttl)
    printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(const sgx_uswitchless_config_t* us_config) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;

  /* Call sgx_create_enclave to initialize an enclave instance */
  /* Debug Support: set 2nd parameter to 1 */

  const void* enclave_ex_p[32] = { 0 };

  enclave_ex_p[SGX_CREATE_ENCLAVE_EX_SWITCHLESS_BIT_IDX] = (const void*)us_config;

  ret = sgx_create_enclave_ex(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL, SGX_CREATE_ENCLAVE_EX_SWITCHLESS, enclave_ex_p);

  if (ret != SGX_SUCCESS) {
    print_error_message(ret);
    return -1;
  }

  return 0;
}


/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
// int initialize_enclave(void)
// {
//     sgx_status_t ret = SGX_ERROR_UNEXPECTED;

//     /* Call sgx_create_enclave to initialize an enclave instance */
//     /* Debug Support: set 2nd parameter to 1 */
//     ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
//     if (ret != SGX_SUCCESS) {
//         print_error_message(ret);
//         return -1;
//     }

//     return 0;
// }

/* OCall functions */
void ocall_print_string(const char* str) {
  /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
  printf("%s", str);
}


void myGetPRFs(size_t number, unsigned char* inputs) {
  
  if (number > (size_t) (1 << 24)) {
    SGXGetPRFs(global_eid, (1 << 24), inputs);
    size_t index = (size_t) (1 << 24) * (size_t) PRFLENGTH;
    size_t tmp = number - (size_t) (1 << 24);
    myGetPRFs(tmp, &inputs[1 << 24]);
  } else {
    SGXGetPRFs(global_eid, number, inputs);
  }
}

/* Application entry */
int SGX_CDECL main(int argc, char* argv[]) {
  int si = sizeof(BT);
  cout << "BT is " << si << " Bytes. Note that you have to set the header in Enclave.config.xml properly and BT in Enclave.edl and Enclave.cpp." << endl;

  cout << "The current configuration is for n=2^";

  if (si == 2) {
    cout << "26." << endl;
  } else {
    cout << "24 or less." << endl;
  }

  if (argc != 4) {
    cout << "Command line parameter: partyNumber RUNS OFFLINE/STRAWMAN" << endl;
    exit(1);
  }

  int party = atoi(argv[1]);
  size_t RUNS = atoi(argv[2]);
  int protocol = atoi(argv[3]);

  /* Configuration for Switchless SGX */
  sgx_uswitchless_config_t us_config = SGX_USWITCHLESS_CONFIG_INITIALIZER;
  us_config.num_uworkers = 2;
  us_config.num_tworkers = 2;

  /* Initialize the enclave */
  if (initialize_enclave(&us_config) < 0) {
    printf("Error: enclave initialization failed\n");
    return -1;
  }


  /* Initialize the enclave */
  /*    if(initialize_enclave() < 0){
      printf("Enter a character before exit ...\n");
      getchar();
      return -1;
  }*/

  /* Utilize edger8r attributes */
  edger8r_array_attributes();
  edger8r_pointer_attributes();
  edger8r_type_attributes();
  edger8r_function_attributes();

  /* Utilize trusted libraries */
  ecall_libc_functions();
  ecall_libcxx_functions();

  //ecall_thread_functions();
  if (protocol == 1) {//Strawman
    double CPUTime = 0.0;
    auto totalTime = clock_start();
    int port = 9876;
    NetIO* io;

    if (party == 1) { //Sender
      cout << "n = " << size_t(NELEMENTS) << endl;
      size_t RAM = (size_t) NELEMENTS * (size_t) PRFLENGTH;
      cout << "Asking for " << size_t(RAM) << " Byte of RAM" << endl;
      unsigned char* inputs = (unsigned char*) malloc(RAM);

      if (inputs == NULL) {
        cout << "Malloc failed." << endl;
        exit(-11);
      }

      PRG prg(fix_key);
      prg.random_data(inputs, RAM);

      cout << "Party " << int(party) << ": computing PRFs..." << endl;

      auto timeStamp = clock_start();
      totalTime = clock_start();
      myGetPRFs(NELEMENTS, inputs);
      CPUTime += time_from(timeStamp);


      io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);
      io->sync();
      cout << "Party " << int(party) << ": sending PRFs" << endl;
      mySend(io, (char*) inputs, RAM);
      
      free(inputs);
    } else { //Receiver

      size_t RAM = (size_t) NELEMENTS * (size_t) PRFLENGTH;
      cout << "Asking for " << size_t(RAM) << " Byte of RAM" << endl;
      unsigned char* inputs = (unsigned char*) malloc(RAM);

      if (inputs == NULL) {
        cout << "Malloc failed." << endl;
        exit(-1);
      }

      PRG prg(fix_key);
      prg.random_data(inputs, RAM);

      cout << "Party " << int(party) << ": computing PRFs..." << endl;
      totalTime = clock_start();
      auto timeStamp = clock_start();
      myGetPRFs(NELEMENTS, inputs);
      CPUTime += time_from(timeStamp);

      cout << "Party " << int(party) << ": hashing PRFs..." << endl;
      timeStamp = clock_start();

      std::unordered_map<string, int> ht;

      for (size_t i = 0; i < NELEMENTS; i++) {
        ht[hex2String(&inputs[i * (size_t)PRFLENGTH], (size_t) PRFLENGTH)] = 1;
      }

      free(inputs);
      
      CPUTime += time_from(timeStamp);

      io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);
      io->sync();

      cout << "Party " << int(party) << ": receiving PRFs" << endl;
      unsigned char* otherPRFs = (unsigned char*) malloc(RAM);
      if (otherPRFs == NULL) {
	cout <<"otherPRFs malloc failed."<<endl;
	exit(-1);
      }
      myRecv(io, (char*) otherPRFs, RAM);

      //Computing intersection
      cout << "Party " <<   int(party) << ": Computing intersection..." << endl;
      timeStamp = clock_start();
      size_t matches = 0;

      for (size_t i = 0; i < NELEMENTS; i++) {
        if (ht[hex2String(&otherPRFs[i * (size_t)PRFLENGTH], (size_t)PRFLENGTH)] == 1) {
          matches ++;
        }
      }

      cout << "Matches: " << size_t(matches) << endl;
      CPUTime += time_from(timeStamp);

    }

    cout << "Party " << int(party) << ": CPU time: " << CPUTime / 1000 << " ms, total time: " << time_from(totalTime) / 1000 << " ms, data sent: " << ((double) io->counter) / (1024 * 1024) << " MByte" << endl;


  } else { //Offline
    BT* r = (BT*) calloc(1, BUCKETS * BETA * sizeof(BT));
    BT* s = (BT*) calloc(1, BUCKETS * BETA * sizeof(BT));

    printf("Starting benchmark for %s with BUCKETS=%ld, BETA=%ld. Doing %d runs.\n", myName, BUCKETS, BETA, RUNS);

#define FACTOR 100

    auto start = clock_start();

    for (size_t j = 0; j < RUNS; j++) {
      for (size_t i = 0; i < FACTOR; i++) {
        SGXBench(global_eid, BUCKETS / FACTOR, BETA, party,  r, s, MODULUS);
      }
    }

    auto elapsed = time_from(start);
    cout << "Total time to generate all quadruples: " << elapsed / (RUNS * 1000) << " ms" << endl;
    cout << "Time per quadruple: " << 1000UL * elapsed / (1UL * BUCKETS * BETA * RUNS) << " ns" << endl;
    cout << "Time per bucket: " << 1000UL * elapsed / (1UL * BUCKETS * RUNS) << " ns" << endl;
  }

  /* Destroy the enclave */
  sgx_destroy_enclave(global_eid);

  printf("Info: SampleEnclave successfully returned.\n");

  return 0;
}

