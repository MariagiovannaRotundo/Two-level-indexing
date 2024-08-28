#!/bin/sh

tmp=$1
nameinput="${tmp%.txt}"
filename="${nameinput##*/}"

path="./storage/rear_coding_storage/"

for i in 4 8 16 32
do
   ./indexing_level_tests/test_louds "${path}${filename}_first_${i}K.txt" $i 4
   ./indexing_level_tests/test_dfuds "${path}${filename}_first_${i}K.txt" $i 4
   ./indexing_level_tests/test_array "${path}${filename}_first_${i}K.txt" $i 4
   ./indexing_level_tests/test_PDT "${path}${filename}_first_${i}K.txt" $i 4
   ./indexing_level_tests/test_FST "${path}${filename}_first_${i}K.txt" $i 4
   ./indexing_level_tests/generate_coco "${path}${filename}_first_${i}K.txt" $i
   ./indexing_level_tests/test_coco "${path}${filename}_first_${i}K.txt" $i 4
done


if [ ! -d coco_tries ]; then
   mkdir coco_tries
fi

mv *".bin" coco_tries


if [ ! -d results ]; then
        mkdir results
fi

for csvfile in *.csv;
do
   csvfilename="${csvfile%.csv}"
   if [ -f "results/${csvfile}" ]; then
      cat results/${csvfile} ${csvfile} > results/${csvfilename}_tmp.csv
      mv results/${csvfilename}_tmp.csv results/${csvfile}
   else
      mv ${csvfile} results
   fi
done

