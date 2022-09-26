project(she-seal)
cmake_minimum_required(VERSION 3.17)

include_directories(../emp-tool /usr/local/opt/gmp/include)

find_package(SEAL 3.6 REQUIRED)
#find_package(OpenMP REQUIRED)
find_package(OpenSSL REQUIRED)
#find_package(GMP REQUIRED)



add_executable(she-seal she-seal.cpp)

target_link_libraries(she-seal SEAL::seal OpenSSL::SSL)
#target_link_libraries(she-seal SEAL::seal OpenMP::OpenMP_CXX)

