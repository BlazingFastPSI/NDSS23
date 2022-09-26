template<class... Ts>
void run_function(void *function, const Ts&... args) {	
	reinterpret_cast<void(*)(Ts...)>(function)(args...);
}

template<typename T>
void inline delete_array_null(T * ptr){
	if(ptr != nullptr) {
		delete[] ptr;
		ptr = nullptr;
	}
}

inline void error(const char * s, int line, const char * file) {

  printf("Error: %s\n", s);
}

inline void parse_party_and_port(char ** arg, int * party, int * port) {
	*party = atoi (arg[1]);
	*port = atoi (arg[2]);
}

template <typename T>
inline T bool_to_int(const bool *data) {
    T ret {};
    for (size_t i = 0; i < sizeof(T)*8; ++i) {
        T s {data[i]};
        s <<= i;
        ret |= s;
    }
    return ret;
}

template<typename T>
inline void int_to_bool(bool * data, T input, int len) {
	for (int i = 0; i < len; ++i) {
		data[i] = (input & 1)==1;
		input >>= 1;
	}
}

inline block bool_to_block(const bool * data) {
	return makeBlock(bool_to_int<uint64_t>(data+64), bool_to_int<uint64_t>(data));
}

inline void  block_to_bool(bool * data, block b) {
	uint64_t* ptr = (uint64_t*)(&b);
	int_to_bool<uint64_t>(data, ptr[0], 64);
	int_to_bool<uint64_t>(data+64, ptr[1], 64);
}

inline bool file_exists(const std::string &name) {
	 return false;
}


