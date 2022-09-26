# REQUIREMENTS
# CMake >= 3.12
# clang >= 5.0

# ADD NECESSARY DIRECTORIES TO THE PATH
# export PATH="...:$PATH"

# PATH FOR CLANG COMPILER
# IF CLANG IS NOT AVAILABLE, G++ IS ALSO USABLE, BUT SLOWER
export CC=/usr/bin/clang 
export CXX=/usr/bin/clang++

# SET THE INSTALL DIR
INSTALL_DIR=~/.local

#git clone https://github.com/microsoft/SEAL.git
cd SEAL
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR
cmake --build build
cmake --install build
