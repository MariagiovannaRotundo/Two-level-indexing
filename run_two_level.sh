#!/bin/sh

tmp=$1
file="${tmp%.txt}"
filename="${file##*/}"

#0 if it is unset and take the new generated queries, use our queries otherwise
q=${2:0}

if [[ $q == 0 ]]; then
   for i in 4 8 16 32
   do
      blocksize=$(($i*1024))
      ./two_level_tests/generate_test_array "${file}_B_${blocksize}.txt" $i "${file}-query.txt" "two_level_results"
      ./two_level_tests/generate_test_PT "${file}_B_${blocksize}.txt" $i "${file}-query.txt" "two_level_results"
   done
else
   for i in 4 8 16 32
   do
      blocksize=$(($i*1024))
      ./two_level_tests/generate_test_array "${file}_B_${blocksize}.txt" $i "./query_files/${filename}-query.txt" "two_level_results"
      ./two_level_tests/generate_test_PT "${file}_B_${blocksize}.txt" $i "./query_files/${filename}-query.txt" "two_level_results"
   done
fi


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
