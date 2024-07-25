# Whole-dataset compression

To evaluate the performance in compression and decompression time the command "time compression_method dataset" is used.
To evaluate the compressed space in bytes the linux command "ls -al" is used.


# Block-wise compression

For block-wise compression, Rear Coding and Zstd are considered. 

### How to run experiments

``` 
cd compression_tests
chmod +x compile_compression.sh prepare_compression.sh execute_compression.sh
./compile_compression.sh
./prepare_compression.sh dataset.txt
./execute_compression.sh dataset.txt num_repetitions
```

### Results
You will find results in the output file "block-wise_decompression_stats.csv"



