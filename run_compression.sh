tmp=$1
file="${tmp%.txt}"
namefile="${file##*/}"

for i in 4 8 16 32
do
        blocksize=$(($i*1024))
        for (( j = 0 ; j <= $2 ; j += 1 )) ; do
                ./storage_compression_tests/decompress_times_rear_coding "./storage/rear_coding_storage/${namefile}_B_${blocksize}.txt" $i
        done
        for (( j = 0 ; j <= $2 ; j += 1 )) ; do
                ./storage_compression_tests/decompress_times_zstd "./storage/zstd_storage/${namefile}_B_${blocksize}.txt" $i
        done
done

if [ ! -d results ]; then
   mkdir results
fi

mv *".csv" results