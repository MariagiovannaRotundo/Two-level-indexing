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

