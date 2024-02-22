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
#include <time.h>
#include <set>
#include <algorithm>

#include "mmap_level_rear_coding.hpp"


std::string output;

size_t rear_decompress_block(size_t block, char * mmapped_content, size_t logical_block_size, size_t num_string);


int main(int argc, char* argv[]) {

    if (argc != 3){
        std::cout<<"main.cpp fileCompressedStorage logicalBlockSize[KB]"<<std::endl;
        return -1;
    }


    //compute the size ofthe logical blocks
    size_t apprx_logical_b_size = atoi(argv[2])<<10; // *1024 bytes    
    // https://man7.org/linux/man-pages/man3/sysconf.3.html
    auto page_size = sysconf(_SC_PAGE_SIZE);
    size_t logical_block_size = (apprx_logical_b_size & ~(page_size - 1)) + (apprx_logical_b_size%page_size!=0)*page_size;


    MmappedSpace mmap_content;

    //read the ranks of the logical blocks
    std::string file_storage;
    name_reading_blocks_rank(argv[1], file_storage);
    auto b_ranks = read_data_binary<uint64_t>(file_storage, false);

    int fd = open_read_file(argv[1]);

    size_t length_storage = (b_ranks.size()-1)*logical_block_size;
    std::cout<<"\nlength_storage "<<length_storage<<std::endl;

    //call the mmap on the storage file and getthe pointer to the content
    mmap_content.open_mapping(fd, 0, length_storage);
    char * mmapped_content = mmap_content.get_mmapped_content();

    size_t num_blocks = b_ranks.size()-1;


    //generate a random sequence of blocks to decompress
    std::set<size_t> blocks; //ordered index of chosen words
    std::vector<size_t> unordered_blocks;

    srand(time(NULL));
    
    //10% of total blocks decompressed to get the statistics
    while(blocks.size()<(num_blocks/10)){
        auto number = rand() % num_blocks;
        auto results = blocks.insert(number);
        if(results.second){
            unordered_blocks.push_back(number); //contain the number/index of the block to decompress
        }
    }

    
    size_t ns;
    size_t rear_length;

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i=0; i < unordered_blocks.size(); i++){
        rear_length = rear_decompress_block(unordered_blocks[i], mmapped_content, logical_block_size, 
            b_ranks[unordered_blocks[i]+1] - b_ranks[unordered_blocks[i]]);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    volatile auto rear = rear_length;

    std::cout << "\naverage time to decompress a block: "<<ns/unordered_blocks.size()<<std::endl; 

    //close the mmap anf the files
    mmap_content.close_mapping(length_storage);
    close_file(fd);
    

    return 0;
}



size_t rear_decompress_block(size_t block, char * mmapped_content, size_t logical_block_size, size_t num_string){

    
    auto data_ptr = mmapped_content + block * logical_block_size;
    data_ptr += strlen(data_ptr) + 1; //second (compressed (rear, data)) string 

    size_t rear_length;
    for (int j = 1; j < num_string; j++) {  
        rear_length = decode_int(data_ptr); 
        data_ptr += strlen(data_ptr) + 1; 
    }

    return rear_length;
    
}

