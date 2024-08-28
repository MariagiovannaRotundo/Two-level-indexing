#!/bin/sh

tmp=$1
nameinput="${tmp%.txt}"

for i in 4 8 16 32
do
   ./indexing_level_tests/test_louds "${nameinput}_first_${i}K.txt" $i 4
   ./indexing_level_tests/test_dfuds "${nameinput}_first_${i}K.txt" $i 4
   ./indexing_level_tests/test_array "${nameinput}_first_${i}K.txt" $i 4
   ./indexing_level_tests/test_PDT "${nameinput}_first_${i}K.txt" $i 4
   ./indexing_level_tests/test_FST "${nameinput}_first_${i}K.txt" $i 4
   ./indexing_level_tests/generate_coco "${nameinput}_first_${i}K.txt" $i
   ./indexing_level_tests/test_coco "${nameinput}_first_${i}K.txt" $i 4
done


if [ ! -d coco_tries ]; then
   mkdir coco_tries
fi

mv *".bin" coco_tries


if [ ! -d results ]; then
        mkdir results
fi

for csvfile in ./*.csv;
do
   filename="${csvfile%.csv}"
   if [ -f "results/${csvfile}" ]; then
      cat results/${csvfile} ${csvfile} > results/${filename}_tmp.csv
      mv results/${filename}_tmp.csv results/${csvfile}
   else
      mv results/${csvfile} results
   fi
done

