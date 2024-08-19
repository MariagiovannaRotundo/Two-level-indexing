#!/bin/sh

tmp=$1
file="${tmp%.txt}"
path="${file%/*}"
namefile="${file##*/}"
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
./two_level_tests/generate_query "${file}_B_4096.txt" 4 "${file}" 10000000


# generate storage
for i in 4 8 16 32
do
   blocksize=$(($i*1024))
   echo "${file}_B_${blocksize}.txt"
   ./two_level_tests/save_first_strings "${file}_B_${blocksize}.txt" $i "${file}_first_${i}K"
done
