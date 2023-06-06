#include <fstream>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <cstdio>
#include <iostream>

#include "uncompacted_trie.hpp"
#include "CoCo-trie_v2.hpp"
#include "common_methods.hpp"

#define REPEAT 3 
#define PERCENTAGE 10

//test of CoCo-trie. It reads the serialized CoCo-trie from a file "CoCo_ + blockSize + .bin"
// - input: the file with the string dataset; the block size associated with that dataset; 
//the seed to use to generate the queries
int main(int argc, char* argv[]) {

    if (argc != 4){
        std::cout<<"main.cpp fileWithStrings blockSize seed_query "<<std::endl;
        return -1;
    }

    uint blockSize = atoi(argv[2]);
    std::vector<std::string> word_block; 
    std::vector<std::string> query_word; 
   
    //read distinguishing prefixes of dataset
    std::cout << "\nstart reading with distinguishing" << std::endl;
    read_distinguishing(argv[1], word_block);
    std::cout << "read " <<word_block.size()<<" strings"<< std::endl;

    //generate the string queries
    uint seed = atoi(argv[3]);
    generate_query_dataset_seed(word_block, PERCENTAGE, query_word, seed);


    datasetStats ds = dataset_stats_from_vector(word_block);
    // Global variables
    MIN_CHAR = ds.get_min_char();
    ALPHABET_SIZE = ds.get_alphabet_size();
        
    std::ifstream infile("CoCo_" + std::to_string(blockSize) + ".bin");
    CoCo_v2<> coco(infile);

    //print the memory usage in byte
    std::cout << "Used space: " << coco.size_in_bits()/8 << std::endl;

    //lookup

    size_t key;

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i=0; i< REPEAT; i++){
        for(int i=0; i< query_word.size(); i++){
            key = coco.look_up(query_word[i]);
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    volatile auto temp = key;

    std::cout << "average time for the lookup: "<<ns/(query_word.size()*REPEAT)<<std::endl;

    
    //extract the name of the files bu the paths
    std::string input_name(argv[1]);
    int pos = input_name.find_last_of('/');
    std::string name_in = input_name.substr(pos+1); 

    //output file
    const std::filesystem::path fresult{"./CoCo_results.csv"};
    if(!std::filesystem::exists(fresult)){ 
        FILE* f = std::fopen("./CoCo_results.csv", "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }

        fprintf(f,"name,dataset_trie,block_size,avg_time,space,construction_time\n");// newline
        fprintf(f,"CoCo,%s,%d,%ld,%ld,-\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), coco.size_in_bits()/8);
        std::fclose(f);
    }
    else{
        FILE* f = std::fopen("./CoCo_results.csv", "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }
        fprintf(f,"CoCo,%s,%d,%ld,%ld,-\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), coco.size_in_bits()/8);
        std::fclose(f);
    }
    
    return 0;
}

