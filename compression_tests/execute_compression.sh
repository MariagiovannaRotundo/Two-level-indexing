tmp=$1
file="${tmp%.txt}"
path="${file%/*}"
prefix="*/"
namefile="${file#$path}"
namefile="${namefile#$prefix}"

for i in 4 8 16 32
do
        blocksize=$(($i*1024))
        for (( j = 0 ; j <= $2 ; j += 1 )) ; do
                ./build/decompress_times_rear_coding "rear_coding_storage/${namefile}_B_${blocksize}.txt" $i
        done
        for (( j = 0 ; j <= $2 ; j += 1 )) ; do
                ./build/decompress_times_zstd "zstd_storage/${namefile}_B_${blocksize}.txt" $i
        done
done
