mkdir rear_coding_storage
mkdir zstd_storage

cd rear_coding_storage
for i in 4 8 16 32
do
        ../build/generate_storage_rear_coding ../$1 $i
done

cd ../zstd_storage
for i in 4 8 16 32
do
       ../build/generate_storage_zstd ../$1 $i
done
