#!/bin/sh

tmp=$1
file="${tmp%.txt}"
filename="${file##*/}"

path="./storage/rear_coding_storage/"

query_path=${2:-"queries"}
query_path="${query_path}/"

for i in 4 8 16 32
do
   blocksize=$(($i*1024))
   ./two_level_tests/generate_test_array "${path}${filename}_B_${blocksize}.txt" $i "${query_path}${filename}-query.txt" "two_level_results"
   ./two_level_tests/generate_test_PT "${path}${filename}_B_${blocksize}.txt" $i "${query_path}${filename}-query.txt" "two_level_results"
done


if [ ! -d results ]; then
        mkdir results
fi

for csvfile in *.csv;
do
   csvfilename="${csvfile%.csv}"
   if [ -f "results/${csvfile}" ]; then
      echo "$(tail -n +2 ${csvfile})" > ${csvfile} #remove the first row from the file
      cat results/${csvfile} ${csvfile} > results/${csvfilename}_tmp.csv
      mv results/${csvfilename}_tmp.csv results/${csvfile}
   else
      mv ${csvfile} results
   fi
done
