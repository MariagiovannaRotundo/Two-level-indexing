# Two-level Indexing for Massive String Dictionaries

Repository to reproduce experiments of the two-level approach based on a succinct encoding of Patricia Trie for the index level and fixed-size blocks of strings compressed with Rear Coding for the storage level.

## Dependencies

* Cmake >= 3.12
* Boost >= 1.42
* Compiler supporting C++20
* SDSL (https://github.com/simongog/sdsl-lite)
* zstd (https://github.com/facebook/zstd)
* Sux (https://github.com/vigna/sux)


## Datasets 

The two datasets used for the experiments are available at the link: https://unipiit-my.sharepoint.com/:f:/g/personal/m_rotundo1_studenti_unipi_it/EsqeTw4WZWFEvp49K8G4w1ABQ5ILxjOJnEZjhktlgBRlNg?e=irQxuX

The datasets are compressed with Zstd. After downloading, decompress them with the following command:

```
zstd --ultra -22 -d -M1024MB --long=30 -T16 --adapt
```



## Build the project (Ubuntu)


```
git clone https://github.com/MariagiovannaRotundo/Two-level-indexing
cd Two-level-indexing
chmod +x compile.sh prepare.sh run_indexing.sh run_two_level.sh run_compression.sh run_edge_cache.sh
./compile.sh
```

## Experiments

There are two different types of experiments: the one to evaluate the different approaches in internal memory time and space consumption to index a subset of strings, and the one to evaluate the two-level performance.

For the experiments, new query files are generated by our scripts but if you want to use our queries, please download the `query_files` folder by the same google drive of the datasets in the `Two-level-indexing` folder.


#### How to launch experiments

Create a directory with the dataset to use for the experiments.

1. `./prepare.sh dataset.txt` generates all the files that will be used in the next experiments storing them in the same directory of the input dataset.
2. `./run_indexing.sh dataset.txt` runs experiment about the index level placing the files with the results in a "results" directory and the files with the serialized CoCo-trie in a "coco_tries" directory.
3. `./run_two_level.sh dataset.txt [query_path]` runs the two level experiments writing results in a two_level_results.csv file in "results" directory. To use our queries (after downloaded it) pass the path i.e. the directory name "query_files" with the optional parameter.
4. `./run_compression.sh dataset.txt [repetitions]` runs the block-wise compression experiments by considering rear coding and zstd. The results are places in a block-wise_decompression_stats.csv file in "results" directory. The number of repetitions is optional and by default is set to 1.
5. `./run_edge_cache.sh dataset.txt adversarial_queries [repetitions] [query_path]` runs the experiments related to the edge caching by collecting all the statistics writing them in a edge_caching_results.csv file in the "result" directory. As arguments, we have to pass in input, not only the dataset we are considering, but also a flag to set to 0 if we want to use the adversarial query woarkload (any other value othervise), the number of repetitions (set to 1 by default) and the path where find the queryes (by default we take the ones generated by the `prepare.sh` script).



#### Indexing level experiments
We execute these experiments on the first string of each block of the storage level by creating in advance files with these strings. 
The experiment for CoCo-trie are the only ones divided in two phases: the construction phase and the query phase due to the high time and space consumption of construction phase.


#### Two level experiments

To execute experiments by considering two-level solutions as first thing the storage levels and the queries to use for the evaluation are generated.
By deafault, in the C++ code the number of repetitions of each experiment is fixed to 3.

