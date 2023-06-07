# Two-level Indexing for Massive String Dictionaries

Repository to reproduce experiments of the two-level approach based on a succinct encoding of Patricia Trie for the index level and fixed-size blocks of strings compressed with Rear Coding for the storage level.

## Dependencies

* Cmake >= 3.12
* Boost >= 1.42
* Compiler supporting C++20

## Build the project (Ubuntu)


```
git clone https://github.com/MariagiovannaRotundo/Two-level-indexing
cd Two-level-indexing
chmod +x compile.sh
./compile.sh
````

## Experiments

There are two different types of experiment: the one to evaluate the different approaches in internal memory time and space consumption to index a subset of strings, and the one to evaluate the two-level performance.

#### How to launch experiments

Creare a directory with the dataset to use for the experiments.

1. `./prepare.sh dataset.txt` generate all the files that will be used in the next experiments storing them in the same directory of the input dataset.
2. `./run_indexing.sh dataset.txt` runs experiment about the index level placing the files with the results in a "results" directory and the files with the serialized CoCo-trie in a "coco_tries" directory.
3. `./run_two_level.sh dataset.txt` runs the two level experiments writing results in a two_level_results.csv file in "results" directory.


#### Two level experiments

To execute experiments by considering two-level solutions the first thing done is the generation of the storage levels and of the queries to use for the evaluation 
By deafault, the number of repetitions of each experiment is fixed to 3.

### Indexing level experiments
We execute these experiments on the first string of each block of the storage level by creating in advance files with these strings. 
The experiment for CoCo-trie are the only ones divided in two phases: the construction phase and the query phase de to the high time and space consumption during the construction of the trie.







