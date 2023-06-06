#include <fstream>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <cstdio>
#include <iostream>

#include "uncompacted_trie.hpp"
#include "utils.hpp"
#include "CoCo-trie_v2.hpp"
#include "common_methods.hpp"

#define PERCENTAGE 10
#define REPEAT 3 


//create a CoCo-trie from a set of string and serialize it writing it on file
// - input: the file that contain the set of strings; the size of the logical block
int main(int argc, char* argv[]) {

     if (argc != 3 ){
        std::cout<<"main.cpp fileWithStrings blockSize "<<std::endl;
        return -1;
    }

    uint blockSize = atoi(argv[2]);

    std::vector<std::string> word_block; 
    //read distinguishing prefixes of dataset
    std::cout << "\nstart reading with distinguishing" << std::endl;
    read_distinguishing(argv[1], word_block);
    std::cout << "read " <<word_block.size()<<" strings"<< std::endl;

    
    {
        datasetStats ds = dataset_stats_from_vector(word_block);
        // Global variables
        MIN_CHAR = ds.get_min_char();
        ALPHABET_SIZE = ds.get_alphabet_size();


        auto t0 = std::chrono::high_resolution_clock::now();
        // a trie-index constructed from string keys sorted
        CoCo_v2<> coco(word_block);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "built the trie in " << us << " us " << std::endl;    

        //serialize the CoCo-trie
        std::ofstream outfile("CoCo_" + std::to_string(blockSize) + ".bin");
        coco.serialize(outfile);


        //extract the name of the files by the paths
        std::string input_name(argv[1]);
        int pos = input_name.find_last_of('/');
        std::string name_in = input_name.substr(pos+1); 

        //output file
        const std::filesystem::path fresult{"./CoCo_constr_times.csv"};
        if(!std::filesystem::exists(fresult)){ //if the file fo not exists
            FILE* f = std::fopen("./CoCo_constr_times.csv", "a");
            if(!f) {
                std::perror("File opening failed");
                return EXIT_FAILURE;
            }

            fprintf(f,"name,dataset_trie,block_size,avg_time,space,construction_time\n");// newline
            fprintf(f,"CoCo,%s,%d,-,%ld,%ld\n", name_in.data(), blockSize, coco.size_in_bits()/8, us);
            std::fclose(f);
        }
        else{
            FILE* f = std::fopen("./CoCo_constr_times.csv", "a");
            if(!f) {
                std::perror("File opening failed");
                return EXIT_FAILURE;
            }
            fprintf(f,"CoCo,%s,%d,-,%ld,%ld\n", name_in.data(), blockSize, coco.size_in_bits()/8, us);
            std::fclose(f);
        }

    }

    word_block.clear();
    word_block.shrink_to_fit();

    return 0;
}

