apt update
apt install -y build-essential

# openssl
cd /tmp
wget https://www.openssl.org/source/openssl-3.2.1.tar.gz
tar -zxf openssl-3.2.1.tar.gz && cd openssl-3.2.1
./config
make -j
make install
rm /usr/bin/openssl
ln -s /usr/local/bin/openssl /usr/bin/openssl
ldconfig /usr/local/lib64

# gtest install
apt install -y libgtest-dev
cd /usr/src/gtest
cmake CMakeLists.txt
make
cp ./lib/*.a /usr/lib

