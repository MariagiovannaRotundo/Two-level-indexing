# Two-level Indexing

Repository to reproduce tests of the two-level approach based on a succinct encoding of Patricia Trie for the index layer and fixed-size blocks of strings compressed with Rear Coding for the storage layer.

## Dependencies

* Cmake >= 3.12
* Boost >= 1.42

## Build the project (Ubuntu)


```
git clone https://github.com/MariagiovannaRotundo/Two-level-indexing
cd Two-level-indexing
chmod +x generate_test.sh
./generate_test.sh
````

## Tests

There are two different types of test: the one to evaluate the different approaches in internal memory time and space consumption to index a subset of strings, and the one to evaluate the two-level performance.


### Two level tests

To execute tests by considering two-level solutions the first thing to do is generating the storage layer with `generate_storage` by specifying the dataset and the size of the logical blocks (in KB). Then the queries to use for the evaluation are randomly generated with `generate_query`, specifying the storage layer from which to extract the queries, the size of the blocks (in KB), the file where to store the query and the (optional) number of queries.
After this, tests  `generate_test_array` and `generate_test_PT` can be executed by specifying which storage to use, its block size (in KB), the file with queries, the file where to write the results and the (optional) number of repetitions of the test.

### Indexing level tests
We execute these tests on the first string of each block of the storage level. We can create files with these strings with `save_first_string` in two_level_tests directory. This takes as input the file of the storage level, the size of the logical blocks (in KB), and the output file where to write these strings.

Every test in `indexing_level_tests` (except `generate_coco`) takes in input:

1. the name of the file where the first string of each block are stored;
2. the size of the blocks in KB;
3. the seed to use for the (pseudo) random generation of query set.

The test for CoCo-trie needs files with the serialized CoCo-trie that can be generated with `generate_coco`, which takes in input just the name of the file with the strings to index and the size of the blocks (in KB).







