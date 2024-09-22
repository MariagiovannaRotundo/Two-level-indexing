tmp=$1
file="${tmp%.txt}"
filename="${file##*/}"

path="./storage/rear_coding_storage/"

adversarial=$2 #0 for yes, other values for no

repetitions=${3:-1}

query_path=${4:-"queries"}
query_path="${query_path}/"

queryfilename=${filename}
if [ adversarial == 0 ]; then
    queryfilename="${filename}_adversarial"
fi

for k in 0 1 2
do
    #no cache exsperiments
    for i in 4 8 16 32
    do
        blocksize=$(($i*1024)) 
        ./edge_cache_tests/generate_test_no_caching "${path}${filename}_B_${blocksize}.txt" $i "${query_path}${queryfilename}-query.txt" "edge_caching_results" $k $repetitions
    done

    #edge cache experiments
    for j in 1000000 2000000 4000000 8000000 16000000 32000000 64000000 128000000 #cache sizes
    do
        for i in 4 8 16 32
        do
            blocksize=$(($i*1024)) 
            ./edge_cache_tests/generate_test_caching "${path}${filename}_B_${blocksize}.txt" $i "${query_path}${queryfilename}-query.txt" "edge_caching_results" $j $k $repetitions
        done
    done

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
      rm ${csvfile}
   else
      mv ${csvfile} results
   fi
done
