#!/bin/sh

tmp=$1
file="${tmp%.txt}"
path="${file%/*}"
namefile="${file##*/}"
echo

mkdir storage
mkdir storage/rear_coding_storage
mkdir storage/zstd_storage

#generate zstd storage
for i in 4 8 16 32
do
   ./compression_tests/generate_storage_zstd $1 $i
   blocksize=$(($i*1024))
   storagefile="${namefile}_B_${blocksize}"  
   mv "${storagefile}_zstd.txt" ./storage/zstd_storage
   mv "${storagefile}_blocksRank_zstd.txt" ./storage/zstd_storage
done

#generate rear coding storage
for i in 4 8 16 32
do
   ./compression_tests/generate_storage_rear_coding $1 $i
   blocksize=$(($i*1024))
   storagefile="${namefile}_B_${blocksize}"  
   mv "${storagefile}.txt" ./storage/rear_coding_storage
   mv "${storagefile}_blocksRank.txt" ./storage/rear_coding_storage
done

# generate queries
./two_level_tests/generate_query "${file}_B_4096.txt" 4 "${file}" 10000000


# generate file with the first strings
for i in 4 8 16 32
do
   blocksize=$(($i*1024))
   echo "${file}_B_${blocksize}.txt"
   ./two_level_tests/save_first_strings "${file}_B_${blocksize}.txt" $i "./storage/rear_coding_storage/${namefile}_first_${i}K"
done
