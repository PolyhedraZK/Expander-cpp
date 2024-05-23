apt update
apt install -y build-essential

# gtest install
apt install -y libgtest-dev
cd /usr/src/gtest
cmake CMakeLists.txt
make
cp ./lib/*.a /usr/lib

# mcl
cd /tmp
git clone https://github.com/herumi/mcl
cd mcl
make -j4
make install
