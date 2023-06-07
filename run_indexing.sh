#!/bin/sh

tmp=$1
nameinput="${tmp%.txt}"

for i in 4 8 16 32
do
   ./indexing_level_tests/test_louds "${nameinput}_first_${i}K.txt" $i 5
   ./indexing_level_tests/test_dfuds "${nameinput}_first_${i}K.txt" $i 5
   ./indexing_level_tests/test_array "${nameinput}_first_${i}K.txt" $i 5
   ./indexing_level_tests/test_PDT "${nameinput}_first_${i}K.txt" $i 5
   ./indexing_level_tests/test_FST "${nameinput}_first_${i}K.txt" $i 5
   ./indexing_level_tests/generate_coco "${nameinput}_first_${i}K.txt" $i
   ./indexing_level_tests/test_coco "${nameinput}_first_${i}K.txt" $i 5
done


if [ ! -d results ]; then
   mkdir results
fi

mv *".csv" results

if [ ! -d coco_tries ]; then
   mkdir coco_tries
fi

mv *".bin" coco_tries