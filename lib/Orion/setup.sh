#!/bin/bash
# Script to setup environment
sudo apt update
sudo apt install -y cmake make git clang++-7 libgmp-dev g++ parallel
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-7 1000

git submodule update --init
