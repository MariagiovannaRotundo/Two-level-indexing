#include <fstream>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <cstdio>

#include "fast_succinct_trie/include/fst.hpp"
#include "common_methods.hpp"

#define PERCENTAGE 10
#define REPEAT 3 

//test of fast succinct trie solution
// - input: the file with the string dataset; the block size associated with that dataset; 
//the seed to use to generate the queries
int main(int argc, char* argv[]) {
   
    if (argc != 4){
        std::cout<<"main.cpp fileWithStrings blockSize seed_query"<<std::endl;
        return -1;
    }

    uint blockSize = atoi(argv[2]);

    std::vector<std::string> word_block; 
    //read distinguishing prefixes of dataset
    std::cout << "\nstart reading with distinguishing" << std::endl;
    read_distinguishing(argv[1], word_block);
    std::cout << "read " <<word_block.size()<<" strings"<< std::endl;

    //build the trie
    auto t0 = std::chrono::high_resolution_clock::now();
    fst::Trie trie(word_block);
    auto t1 = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    std::cout << "built the trie in " << us << " us " << std::endl;    

    //print the memory usage in byte
    std::cout << "Used space: " << trie.getMemoryUsage() << std::endl;

    //lookup
    fst::position_t key_id;
    std::vector<std::string> query_word;

    //generate the string queries
    uint seed = atoi(argv[3]);
    generate_query_dataset_seed(word_block, PERCENTAGE, query_word, seed);
    
    word_block.clear();
    word_block.shrink_to_fit();

    t0 = std::chrono::high_resolution_clock::now();
    for(int i=0; i< REPEAT; i++){
        for(int i=0; i< query_word.size(); i++){
            key_id = trie.exactSearch(query_word[i]);
        }
    }
    t1 = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    volatile auto temp = key_id;

    std::cout << "average time for the lookup: "<<ns/(query_word.size()*REPEAT)<<std::endl;

    
    //get the name of the input dataset
    std::string input_name(argv[1]);
    int pos = input_name.find_last_of('/');
    std::string name_in = input_name.substr(pos+1); 

    //output file
    const std::filesystem::path fresult{"./FST_results.csv"};
    if(!std::filesystem::exists(fresult)){ //if the file do not exists
        FILE* f = std::fopen("./FST_results.csv", "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }

        fprintf(f,"name,dataset_trie,block_size,avg_time,space,construction_time\n");
        fprintf(f,"FST,%s,%d,%ld,%ld,%ld\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), trie.getMemoryUsage(),us);
        std::fclose(f);
    }
    else{
        FILE* f = std::fopen("./FST_results.csv", "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }
        fprintf(f,"FST,%s,%d,%ld,%ld,%ld\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), trie.getMemoryUsage(),us);
        std::fclose(f);
    }

    return 0;
}


