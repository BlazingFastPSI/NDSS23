#include "include.h"

//#include <omp.h>
#include <cstring>
#include "seal/seal.h"

using namespace emp;
using namespace std;

#define RUNS 1

void printrB(BT *rBp) {
    for (size_t i = 0; i<BUCKETS; i++) {
        cout <<i<<": ";
        for (size_t j = 0; j<BETA; j++) {
            cout << rBp[i*BETA+j]<<" ";
        }
        cout <<endl;
    }
}

int main(int argc, char **argv)
{
    //omp_set_num_threads(THREADS);

    // Fix randomness for testing
    block seed = makeBlock(7, 8);
    PRG prg;
    prg.reseed(&seed);

    if (argc!=3) {
        cout <<"Must supply party and port number"<<endl;
    }
    int party, port;
    parse_party_and_port(argv, &party, &port);
    
    // Generating homomorphic encryption parameters
    uint64_t poly_mod_degree=1<<13;
    seal::EncryptionParameters params(seal::scheme_type::bfv);
    params.set_poly_modulus_degree(poly_mod_degree);
    vector<seal::Modulus> coeff_mod = seal::CoeffModulus::BFVDefault(poly_mod_degree);
    params.set_coeff_modulus(coeff_mod);
    params.set_plain_modulus(seal::PlainModulus::Batching(poly_mod_degree, 17));
    seal::SEALContext context = seal::SEALContext(params);

    seal::Evaluator evaluator(context);

    seal::BatchEncoder batch_encoder(context);
    uint64_t slot_count = batch_encoder.slot_count();

    string myName = (party==ALICE)?"ALICE":"BOB";
    size_t CT_PER_BUCKET = BETA;
    cout << "Buckets = " << BUCKETS << ", beta = "<< BETA << ", modulus = " << params.plain_modulus().value() << endl;
    cout << "Total number of triples: " << BETA*BUCKETS << endl;

    /*===================*/

    NetIO* io = new NetIO(party==ALICE?nullptr:"127.0.0.1", port);
        
    uint64_t num_ciphers = ceil((float)BUCKETS / poly_mod_degree); 
    uint64_t multiple = BETA;

    double total_time = 0.0;
    double cpuTime = 0.0;
    auto start = clock_start(); // Start timer
    auto cpuTimeStart = start;
    
    if (party == ALICE) { // Alice

        // Generating private-key material for Alice
        seal::KeyGenerator keygen = seal::KeyGenerator(context);
        seal::SecretKey sk = keygen.secret_key();
        seal::Encryptor enc_sym = seal::Encryptor(context, sk);
        seal::Decryptor dec = seal::Decryptor(context, sk);
        
        stringstream str_stream;
        seal::Plaintext pt;
        
        // Generating sA 
        vector<uint64_t> sA_s(slot_count, 0ULL);
        uint64_t transmit_size=0;
        for (int i=0;i<num_ciphers;i++){
            prg.random_data(sA_s.data(), slot_count*sizeof(uint64_t)); // sA is generated randomly
            for (int j=0;j<slot_count;j++){
                sA_s[j] %= params.plain_modulus().value();
            }
            batch_encoder.encode(sA_s, pt);
            seal::Serializable<seal::Ciphertext> ct_ = enc_sym.encrypt_symmetric(pt); // Encrypted using Alice's secret key
            transmit_size += ct_.save(str_stream);
        }

        // Sending to Bob
        string message = str_stream.str();
        char* message_c_str = &message[0];

        string transmit_size_str = string(20-to_string(message.length()).length(), '0') + to_string(message.length());
        char* transmit_size_c_str = &transmit_size_str[0];
	
    cpuTime += time_from(cpuTimeStart); // Adding time of first step
    
        // Sending data to Bob
        io->sync();
        mySend(io, transmit_size_c_str, 20);
        mySend(io, message_c_str, message.length());

        // Wating for Bob ... 

        stringstream response_str_stream;
        while(true){ // Receiving data from Bob in chucks

            io->sync();

            char response_size_c_str[21];
            myRecv(io, response_size_c_str, 20);
            response_size_c_str[20] = '\0';
            uint64_t response_size = atol(response_size_c_str);
            if (response_size == 0){
                break;
            }
            cout << "Alice got the data" << endl;
            char* data = (char*)malloc((response_size+1)*sizeof(char));
            myRecv(io, data, response_size);
            data[response_size] = '\0';
            string data_str(data, response_size);

            response_str_stream << data_str;
        }

	cpuTimeStart = clock_start();

        // Decrypting rA
        seal::Ciphertext cts_rA;
        vector<uint64_t> rA_s;
        for (int i=0;i<num_ciphers;i++){
            for (int j=0;j<multiple;j++){
                cts_rA.load(context, response_str_stream);
                dec.decrypt(cts_rA, pt); // Decrypting using Alice's secret-key
                batch_encoder.decode(pt, rA_s);
            }
        }

	cpuTime += time_from(cpuTimeStart); // Added time for last step

    } else { // Bob
	
        io->sync();
        // Receiving data from Alice
        char transmit_size_c_str[21];
        myRecv(io, transmit_size_c_str, 20);
        transmit_size_c_str[20]='\0';
        uint64_t transmit_size = atoi(transmit_size_c_str);
        // cout << "transmit_size: " << transmit_size << endl;
        cout << "Bob got the data" << endl;
        char* data = (char*)malloc((transmit_size+1)*sizeof(char));
        myRecv(io, data, transmit_size);
        data[transmit_size] = '\0';
        string data_str(data, transmit_size);

	cpuTimeStart = clock_start();

        stringstream str_stream;
        str_stream << data_str;

        stringstream response_str_stream;

        {
            seal::Plaintext pt_r_inv;
            seal::Plaintext pt_s;
            seal::Ciphertext ct;
            
            vector<uint64_t> rB_inv_s = vector<uint64_t>(slot_count, 0ULL);
            vector<uint64_t> sB_s = vector<uint64_t>(slot_count, 0ULL);

            for (uint64_t i=0;i<num_ciphers;i++){
                ct.load(context, str_stream);
                for (uint64_t j=0;j<multiple;j++){
                    
                    // Generating rB and sB
                    prg.random_data(rB_inv_s.data(), slot_count*sizeof(uint64_t)); // rB is generated randomly (the inverse is stored and used)
                    prg.random_data(sB_s.data(), slot_count*sizeof(uint64_t)); // sB is generated randomly
                    for (int k=0;k<slot_count;k++){
                        rB_inv_s[k] %= ((params.plain_modulus().value()-1)+1); // rB in Z*_p
                        sB_s[k] %= params.plain_modulus().value(); // sB in Z_p
                    }
                    batch_encoder.encode(rB_inv_s, pt_r_inv);
                    batch_encoder.encode(sB_s, pt_s);

                    // Deriving the encryption of rA ( = (sA + sB) * rB_inv )
                    evaluator.add_plain_inplace(ct, pt_s);
                    evaluator.multiply_plain_inplace(ct, pt_r_inv);
                    
                    // Modulus switching to reduce ciphertext size after computation
                    evaluator.mod_switch_to_inplace(ct, context.last_context_data()->parms_id());
                    
                    ct.save(response_str_stream);
                }

                if ((i+1)%5000==0) { // Sending data to Alice in chunks

                    string response = response_str_stream.str();
                    char* response_c_str;
                    response_c_str = &response[0];

                    string response_size_str = string(20-to_string(response.length()).length(), '0') + to_string(response.length());
                    char* response_size_c_str = &response_size_str[0];

                    cpuTime += time_from(cpuTimeStart);
                    
                    io->sync();
                    mySend(io, response_size_c_str, 20); //First sending the size of the message
                    mySend(io, response_c_str, response.length()); // Then sending the message

                    response_str_stream.str(std::string()); //Clearing the stringstream
                	
                    cpuTimeStart = clock_start();
                }

            }
        }

	    cout << "Bob finished " << endl;

        string response = response_str_stream.str();
        char* response_c_str = &response[0];

        string response_size_str = string(20-to_string(response.length()).length(), '0') + to_string(response.length());
        char* response_size_c_str = &response_size_str[0];

	cpuTime += time_from(cpuTimeStart);
	
        io->sync();
        mySend(io, response_size_c_str, 20);
        mySend(io, response_c_str, response.length());

        string response_last_str = string(20, '0');
        char* response_last_c_str;
        response_last_c_str = &response_last_str[0];
        io->sync();
        mySend(io, response_last_c_str, 20);

        //END BOB
    }

    total_time += time_from(start);

    cout << "CPU time for party " << myName << ": " << cpuTime/1000<<" ms" << endl;
    cout << "Total time for party " << myName << ": " << total_time/1000 << " ms,"
         << " sent: " << ((double)io->counter)/(1024*1024) << " MByte,"
         << " per tuple: " << ((double)io->counter)/(BUCKETS*BETA) << " Byte" << endl;
    cout << "Time per tuple for party " << myName << ": " << total_time/((size_t)1000*BUCKETS*BETA) << " ms" << endl;


    return 0;
}
