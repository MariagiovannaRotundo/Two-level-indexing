#!/bin/sh

tmp=$1
file="${tmp%.txt}"
filename="${file##*/}"

path="./storage/rear_coding_storage/"

query_path=${2:-"queries"}
query_path="${query_path}/"


for i in 4 16
do
   blocksize1=$(($i*1024)) #4, 16
   ./two_level_tests/generate_test_PT "${path}${filename}_B_${blocksize1}.txt" $i "${query_path}${filename}-query.txt" "two_level_results"
   blocksize2=$(($i*1024*2)) #8, 32
   ./two_level_tests/generate_test_array "${path}${filename}_B_${blocksize2}.txt" $(($i*2)) "${query_path}${filename}-query.txt" "two_level_results"
done

for i in 4 16
do
   blocksize1=$(($i*1024)) #4, 16
   ./two_level_tests/generate_test_array "${path}${filename}_B_${blocksize1}.txt" $i "${query_path}${filename}-query.txt" "two_level_results"
   blocksize2=$(($i*1024*2)) #8, 32
   ./two_level_tests/generate_test_PT "${path}${filename}_B_${blocksize2}.txt" $(($i*2)) "${query_path}${filename}-query.txt" "two_level_results"
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
