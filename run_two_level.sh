#!/bin/sh

tmp=$1
file="${tmp%.txt}"
path="${file%/*}"
prefix="*/"
namefile="${file#$prefix}"

for i in 4 8 16 32
do
   blocksize=$(($i*1024))
   ./two_level_tests/generate_test_array "${path}/${namefile}_B_${blocksize}.txt" $i "${path}/${namefile}-query.txt" "two_level_results"
   ./two_level_tests/generate_test_PT "${path}/${namefile}_B_${blocksize}.txt" $i "${path}/${namefile}-query.txt" "two_level_results"
  
done


if [ ! -d results ]; then
   mkdir results
fi

mv *".csv" results