#!/bin/sh

tmp=$1
file="${tmp%.txt}"
path="${file%/*}"
prefix="*/"
namefile="${file#$prefix}"
echo 

for i in 4 8 16 32
do
   ./two_level_tests/generate_storage $1 $i
   blocksize=$(($i*1024))
   storagefile="${namefile}_B_${blocksize}"  
   mv "${storagefile}.txt" ${path}
   mv "${storagefile}_blocksRank.txt" ${path}
done

# generate queries
./two_level_tests/generate_query "${path}/${namefile}_B_4096.txt" 4 "${path}/${namefile}" 100000 #10000000


# generate storage
for i in 4 8 16 32
do
   blocksize=$(($i*1024))
   echo "${namefile}_B_${blocksize}.txt"
   ./two_level_tests/save_first_strings "${path}/${namefile}_B_${blocksize}.txt" $i "${namefile}_first_${i}K"
   mv "${namefile}_first_${i}K.txt" ${path}
done