#!/bin/sh

git submodule update --init --recursive
cd indexing_level_tests/CoCo-trie
bash ./lib/adapted_code/move.sh
cd ../path_decomposed_tries/
git submodule init
git submodule update
cd ../..
cmake .
make -j 8
