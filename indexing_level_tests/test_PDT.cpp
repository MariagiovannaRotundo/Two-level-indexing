#include <fstream>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include <filesystem>
#include <cstdio>

#include "tries/path_decomposed_trie.hpp"
#include "tries/compressed_string_pool.hpp"
#include "succinct/mapper.hpp"

#include "common_methods.hpp"

#define PERCENTAGE 10
#define REPEAT 3

//test of Path Decomposed Trie solution
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

    //true for the lexicographic search
    succinct::tries::path_decomposed_trie<succinct::tries::compressed_string_pool, true> *trie;
    succinct::util::stl_string_adaptor adaptor;

    //build the trie
    auto t0_build = std::chrono::high_resolution_clock::now();
    trie = new succinct::tries::path_decomposed_trie<succinct::tries::compressed_string_pool, true>(word_block, adaptor); 
    auto t1_build = std::chrono::high_resolution_clock::now();
    auto us_build = std::chrono::duration_cast<std::chrono::microseconds>(t1_build - t0_build).count();

    std::cout << "built the trie in " << us_build << " us " << std::endl;
    
    //print the memory usage in byte
    std::cout << "size of the trie: "<< succinct::mapper::size_of(*trie) << " bytes"<< std::endl;
    
    //lookup
    
    size_t index = 0;

    //generate the string queries
    std::vector<std::string> query_word;
    uint seed = atoi(argv[3]);
    generate_query_dataset_seed(word_block, PERCENTAGE, query_word, seed);
    

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i=0; i< REPEAT; i++){
        for(int i=0; i< query_word.size(); i++){
            index += (*trie).index(query_word[i]); 
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    volatile auto temp = index;

    std::cout << "\naverage time for the lookup: "<<ns/(query_word.size()*REPEAT)<<std::endl;


    //extract the name of the files by the paths
    std::string input_name(argv[1]);
    int pos = input_name.find_last_of('/');
    std::string name_in = input_name.substr(pos+1); 

    //output file 
    const std::filesystem::path fresult{"./PDT_results.csv"};
    if(!std::filesystem::exists(fresult)){ //if the file fo not exists
        FILE* f = std::fopen("./PDT_results.csv", "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }

        fprintf(f,"name,dataset_trie,block_size,avg_time,space,construction_time\n");// newline
        fprintf(f,"PDT,%s,%d,%ld,%ld,%ld\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), succinct::mapper::size_of(*trie),us_build);
        std::fclose(f);
    }
    else{
        FILE* f = std::fopen("./PDT_results.csv", "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }
        fprintf(f,"PDT,%s,%d,%ld,%ld,%ld\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), succinct::mapper::size_of(*trie),us_build);
        std::fclose(f);
    }

    return 0;
}



