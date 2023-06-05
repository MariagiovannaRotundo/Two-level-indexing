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

#include "patricia_trie.hpp"

#define REPEAT 3

//read strings from file
// - input: file to which read the strings; vector of strings where to load them
int read_strings_from_file(char* filename, std::vector<std::string>& wordList){

    std::ifstream ifs(filename, std::ifstream::in); 
    if (!ifs){
        std::cout<<"cannot open "<<filename<<"\n"<<std::endl;
        return -1;
    }

    std::string word;
    //read all the words by skipping empty strings
    while (getline(ifs, word)){
        if (word.size() > 0 && word[word.size()-1] == '\r'){
            word = word.substr(0, word.size()-1);
        }
        if (word == "") continue;
        wordList.push_back(word); 
    }

    return 0;
}


//ecute test of the two-level approach based on Patricia Trie for the indexing level
// - input: file where the storage level is; approximate size of the logical block expressed in KB; file that
// contain the strings to use for queries; name of file where to write the results (.csv is appended to the specifyed name);
// how many times repeat the queries (optional. default = 3)
int main(int argc, char* argv[]) {

    if (argc != 5 && argc != 6){
        std::cout<<"main.cpp fileStorage logicalBlockSize[KB] fileQuery outputFileResults [repetitions]"<<std::endl;
        return -1;
    }

    size_t repetitions = REPEAT;

    if(argc == 6){
        repetitions = atoi(argv[5]);
    }

    size_t apprx_logical_b_size = atoi(argv[2])<<10; // *1024 bytes
    
    // https://man7.org/linux/man-pages/man3/sysconf.3.html
    auto page_size = sysconf(_SC_PAGE_SIZE);
    size_t logical_block_size = (apprx_logical_b_size & ~(page_size - 1)) + (apprx_logical_b_size%page_size!=0)*page_size;

    MmappedSpace mmap_content;
    size_t us;

    //output file will be a .csv
    std::string output_file(argv[4]);
    output_file.append(".csv");

    //open the output file
    const std::filesystem::path fresult{output_file}; //array_on_sampling.csv
    FILE* f;
    if(!std::filesystem::exists(fresult)){ //if the file fo not exists
        f = std::fopen(output_file.data(), "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }

        fprintf(f,"name,dataset,logical_b_size,avg_time,space,disk_space,construction_time\n");// newline
    }
    else{
        f = std::fopen(output_file.data(), "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }
    }    

    
    LOUDS_PatriciaTrie louds_pt;

    // read the ranks associated with the blocks of the storage level
    std::string file_storage;
    name_reading_blocks_rank(argv[1], file_storage);
    auto b_ranks = read_data_binary<uint64_t>(file_storage, false);

    //open the file with the storage level
    int fd = open_read_file(argv[1]);

    size_t length_storage = (b_ranks.size()-1)*logical_block_size;
    std::cout<<"length_storage "<<length_storage<<std::endl;

    //create the mapping to read the strings
    mmap_content.open_mapping(fd, 0, length_storage);
    char * mmapped_content = mmap_content.get_mmapped_content();
    
    //take the first string of each logical block and create the particia trie and the rank/select data structures

    sdsl::int_vector<> blocks_rank(b_ranks.size());
        
    int height; //will contain the height of the trie

    {
        std::vector<std::string> first_strings;
        //get the first string of each logical block
        for(size_t i=0; i<b_ranks.size()-1; i++){ //the last value is associated at the end of the last block 
            std::string s(mmapped_content + i * logical_block_size);
            first_strings.push_back(s);
        }
        // create the trie
        auto t0 = std::chrono::high_resolution_clock::now();
        louds_pt.build_trie(first_strings);
        auto t1 = std::chrono::high_resolution_clock::now();
        us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "\nbuilt the trie in " << us << " us" << std::endl;    

        for(size_t i=0; i<b_ranks.size(); i++){
            //here we save the number of logical blocks that preceede the current one
            blocks_rank[i] = b_ranks[i];
        }

        b_ranks.clear();
        b_ranks.shrink_to_fit();

        //get the height of the trie
        height = louds_pt.get_heigth(first_strings);
        std::cout << "\nheight of the trie: " << height << std::endl;

        first_strings.clear();
        first_strings.shrink_to_fit();
    }

    sdsl::util::bit_compress(blocks_rank);

    auto louds = louds_pt.getLouds();

    //groups of 64 bits (so we count how many groups we need for louds representation)
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

    // create the select_0 data structure
    sux::bits::SimpleSelectZeroHalf louds_select_0(bitvect, louds.size());

    //size of the Patricia Trie
    size_t size = louds_pt.getSize(louds_select_0, blocks_rank);  
    std::cout << "size of the trie " << size << std::endl;
    std::cout << "size on disk " << length_storage << std::endl;
    std::cout << "total size " << size +  length_storage << std::endl;


    //here queries start

    size_t rank;
    //stack used to the upward traversal
    std::vector<size_t> seen_arcs;
    seen_arcs.resize(height);

    //vector with the queries
    std::vector<std::string> query_word;
    //read from file the strings to use for queries
    read_strings_from_file(argv[3], query_word);

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int j=0; j<repetitions; j++){
        for(size_t i=0; i< query_word.size(); i++){

            rank = louds_pt.lookup(mmap_content, query_word[i], louds_select_0, seen_arcs, blocks_rank, logical_block_size);

            //to check the correctness of the function in the case all searched strings are in the dataset
            if(rank==size_t(-1)){
                std::cout << "looking for "<<query_word[i]<<std::endl;
                std::cout << "string NOT found"<<std::endl;
                return 0;
            }
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    
    volatile auto temp = rank;

    std::cout << "\naverage time for the lookup: "<<ns/(query_word.size()*repetitions)<<"\n"<<std::endl;

    fprintf(f,"PT-LOUDS,%s,%ld,%ld,%ld,%ld,%ld\n", argv[1], logical_block_size, ns/(query_word.size()*repetitions), size, length_storage, us);
            
    //close the mmap anf the files
    mmap_content.close_mapping(length_storage);

    close_file(fd);
    std::fclose(f);

    return 0;
}
