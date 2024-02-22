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

#include "storage_level_rear_coding.hpp"

// to generate a storage composed of string stored in blocks where each block is compressed with rear coding
// INPUT : file containing the dictionary of strings; approximate logical block size expressed in KB (e.g 4096 bytes -> 4 KB)
int main(int argc, char* argv[]) {
   
    if (argc != 3){
        std::cout<<"main.cpp fileWithStrings logicalBlockSize[KB]"<<std::endl;
        return -1;
    }

    std::vector<size_t> blocks_rank;

    //compute the logical block size af around the one indicated beeing a multiple of the physical block size
    size_t apprx_logical_b_size = atoi(argv[2])<<10; // *1024 bytes
    // https://man7.org/linux/man-pages/man3/sysconf.3.html
    auto page_size = sysconf(_SC_PAGE_SIZE);
    size_t logical_block_size = (apprx_logical_b_size & ~(page_size - 1)) + (apprx_logical_b_size%page_size!=0)*page_size;

    std::cout<<"logical_block_size "<<logical_block_size<<std::endl;

    //create a storage with the expressed logical block size
    create_storage(argv[1], logical_block_size, blocks_rank);

    //write on a separate file the block ranks of the created storage
    write_file_block_ranks(argv[1], blocks_rank, logical_block_size);

    return 0;
}





