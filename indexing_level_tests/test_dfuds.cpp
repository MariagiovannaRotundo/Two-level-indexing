#include <fstream>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "dfuds_patricia_trie.hpp"
#include "common_methods.hpp"

#define PERCENTAGE 10
#define REPEAT 3

//test the Patricia Trie succincly encoded with DFUDS
// - input: the file with the string dataset; the block size associated with that dataset; 
//the seed to use to generate the queries
int main(int argc, char* argv[]) {


    if (argc != 4){
        std::cout<<"main.cpp fileWithStrings blockSize seed_query"<<std::endl;
        return -1;
    }

    //block size to which the input dataset is associated with
    uint blockSize = atoi(argv[2]);

    std::vector<std::string> word_block; 
    std::vector<std::string> query_word; 
    
    //read the distinguishing prefixes of the input dataset
    std::cout << "\nstart reading distinguishing prefixes" << std::endl;
    read_distinguishing(argv[1], word_block);
    std::cout << "read " <<word_block.size()<<" strings"<< std::endl;

    //generate the string queries
    uint seed = atoi(argv[3]);
    generate_query_dataset_seed(word_block, PERCENTAGE, query_word, seed);


    {   
        DFUDS_PatriciaTrie dfuds_pt;

        int height;
        size_t us;
        {
            
            // create the trie
            std::cout << "start the building of the trie "<< std::endl;    
            auto t0 = std::chrono::high_resolution_clock::now();
            dfuds_pt.build_trie(word_block);
            auto t1 = std::chrono::high_resolution_clock::now();
            us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            std::cout << "\nbuilt the trie in " << us << " us" << std::endl;    

            dfuds_pt.compute_heigth(word_block);
            height = dfuds_pt.get_heigth();

            word_block.clear();
            word_block.shrink_to_fit();
        }

        //space used by Patricia Trie
        size_t size = dfuds_pt.getSize();  
        std::cout << "size of the trie " << size << std::endl;

        //lookup
        
        int rank = 0;

        //vectors used to upward traverse the trie
        std::vector<size_t> seen_arcs;
        std::vector<size_t> arg_close;
        std::vector<size_t> seen_nodes;
        seen_arcs.resize(height);
        arg_close.resize(height);
        seen_nodes.resize(height);

    	
        auto t0 = std::chrono::high_resolution_clock::now();
        for(int i=0; i< REPEAT; i++){
            for(int i=0; i< query_word.size(); i++){
                rank = dfuds_pt.lookup_no_strings(query_word[i], seen_arcs, arg_close, seen_nodes);
            }
        }
        auto t1= std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

        volatile auto temp = rank;

        std::cout << "\naverage time for the lookup (ns): "<<ns/(query_word.size()*REPEAT)<<std::endl;
  

        //get the name of the input dataset
        std::string input_name(argv[1]);
        int pos = input_name.find_last_of('/');
        std::string name_in = input_name.substr(pos+1); 

        //output file
        const std::filesystem::path fresult{"./patricia_results.csv"}; 
        if(!std::filesystem::exists(fresult)){ //if the file fo not exists
            FILE* f = std::fopen("./patricia_results.csv", "a");
            if(!f) {
                std::perror("File opening failed");
                return EXIT_FAILURE;
            }

            fprintf(f,"name,dataset_trie,block_size,avg_time,space,construction_time\n");// newline
            fprintf(f,"PT-DFUDS,%s,%d,%ld,%ld,%ld\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), size, us);
            
            std::fclose(f);
        }
        else{
            FILE* f = std::fopen("./patricia_results.csv", "a");
            if(!f) {
                std::perror("File opening failed");
                return EXIT_FAILURE;
            }
            fprintf(f,"PT-DFUDS,%s,%d,%ld,%ld,%ld\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), size, us);
            
            std::fclose(f);
        }
    }


    return 0;
}


