#ifndef EMP_UTILS_H__
#define EMP_UTILS_H__
#include <string>
#include "block.h"
#include <sstream>
#include <cstddef>//https://gcc.gnu.org/gcc-4.9/porting_to.html
#include "constants.h"
#include <chrono>
#define macro_xstr(a) macro_str(a)
#define macro_str(a) #a

using std::string;

namespace emp {
template<typename T>
void inline delete_array_null(T * ptr);

inline void error(const char * s, int line = 0, const char * file = nullptr);

template<class... Ts>
void run_function(void *function, const Ts&... args);

inline void parse_party_and_port(char ** arg, int * party, int * port);



//Conversions
template<typename T>
inline T bool_to_int(const bool * data);

template<typename T>
inline void int_to_bool(bool * data, T input, int len);

block bool_to_block(const bool * data);

void block_to_bool(bool * data, block b);

bool file_exists(const std::string &name);

#include "utils.hpp"
}
#endif// UTILS_H__
