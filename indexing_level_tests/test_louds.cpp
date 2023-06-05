#include <string>
#include <fstream>
#include <vector>
#include <string.h>
#include <filesystem>
#include <cstdio>
#include <sdsl/vectors.hpp>
#include <sux/bits/SimpleSelectZeroHalf.hpp> //https://sux.di.unimi.it/html/

#include "louds_patricia_trie.hpp"
#include "common_methods.hpp"


#define PERCENTAGE 10
#define REPEAT 3


//test the Patricia Trie succincly encoded with LOUDS
// - input: the file with the string dataset; the block size associated with that dataset; 
//the seed to use to generate the queries
int main(int argc, char* argv[]){
    
    if (argc != 4){
        std::cout<<"main.cpp fileWithStrings blockSize seed_query"<<std::endl;
        return -1;
    }

    //block size to which the input dataset is associated with
    uint blockSize = atoi(argv[2]);

 
    std::vector<std::string> word_block; 
    std::vector<std::string> query_word; 

    //read the distinguishing prefixes of the input dataset
    std::cout << "\nstart reading with distinguishing" << std::endl;
    read_distinguishing(argv[1], word_block);
    std::cout << "read " <<word_block.size()<<" strings"<< std::endl;

    //generate the string queries
    uint seed = atoi(argv[3]);
    generate_query_dataset_seed(word_block, PERCENTAGE, query_word, seed);
    

    {

        LOUDS_PatriciaTrie louds_pt;

        int height;
        size_t us;
        {      
            // create the trie
            auto t0 = std::chrono::high_resolution_clock::now();
            louds_pt.build_trie(word_block);
            auto t1 = std::chrono::high_resolution_clock::now();
            us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            std::cout << "built the trie in " << us << " us" << std::endl;    
            
            height = louds_pt.get_heigth(word_block);
        }
        
        
        //create the select_0 data structure
        auto louds = louds_pt.getLouds();

        uint64_t * bitvect = (uint64_t *) malloc (sizeof(uint64_t) * (louds.size() / 64 + 1)) ;
    
        //sux counts the bits starting from the end of the sequence
        for(int i =0 ; i<louds.size(); i++){
            int final_pos = (i / 64 + 1) * 64 -1;
            int initial_pos = (i / 64) * 64; 

            uint64_t b;

            if(louds.size() > final_pos) //block with less than 64 bits in louds (last block)
                b = louds[final_pos - (i - initial_pos)];
            else
                b = louds[louds.size() - (i - initial_pos) - 1 ];
                // b = louds[louds.size() - i - 1];

            if(i % 64  == 0){ //new 64-bit word
                bitvect[i / 64] = b;
            }
            else{
                bitvect[i / 64] =  (bitvect[i / 64] << 1) | b; //bitvect[i / 64] |= b << i % 64  - to not reverse the bits
            }
        }

        sux::bits::SimpleSelectZeroHalf louds_select_0(bitvect, louds.size());

        

        //space used by Patricia Trie
        size_t size = louds_pt.getSize_sux(louds_select_0);  
        std::cout << "\nsize of the trie " << size << std::endl;


        //lookup
        
        int rank = 0;

        //vector used to upward traverse the trie
        std::vector<int> seen_arcs;
        seen_arcs.resize(height);

       
        auto t0 = std::chrono::high_resolution_clock::now();
        for(int i=0; i< REPEAT; i++){
            for(int i=0; i< query_word.size(); i++){
                rank = louds_pt.lookup_no_strings(query_word[i], louds_select_0, seen_arcs);
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
        const std::filesystem::path fresult{"./patricia_results.csv"}; //PT_on_sampling.csv
        if(!std::filesystem::exists(fresult)){ //if the file fo not exists
            FILE* f = std::fopen("./patricia_results.csv", "a");
            if(!f) {
                std::perror("File opening failed");
                return EXIT_FAILURE;
            }

            fprintf(f,"name,dataset_trie,block_size,avg_time,space,construction_time\n");// newline
            fprintf(f,"PT-LOUDS,%s,%d,%ld,%ld,%ld\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), size,us);
            std::fclose(f);
        }
        else{
            FILE* f = std::fopen("./patricia_results.csv", "a");
            if(!f) {
                std::perror("File opening failed");
                return EXIT_FAILURE;
            }
            fprintf(f,"PT-LOUDS,%s,%d,%ld,%ld,%ld\n", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), size,us);
            std::fclose(f);
        }
    }
    
    return 0;
}


