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

#include "mmap_level_fixed_blocks.hpp"


//save on file the first string of each logical block of the storage level
// - input: the file with the storage level; the logical block size expressed in KB; the output file
int main(int argc, char* argv[]) {

    if (argc != 4){
        std::cout<<"main.cpp fileStorage logicalBlockSize[KB] outputFileResults"<<std::endl;
        return -1;
    }

    size_t apprx_logical_b_size = atoi(argv[2])<<10; // *1024 bytes
    
    // https://man7.org/linux/man-pages/man3/sysconf.3.html
    auto page_size = sysconf(_SC_PAGE_SIZE);
    size_t logical_block_size = (apprx_logical_b_size & ~(page_size - 1)) + (apprx_logical_b_size%page_size!=0)*page_size;

    MmappedSpace mmap_content;
    size_t us;

    //output file name
    std::string output_file(argv[3]);
    output_file.append(".txt");

    FILE* f = std::fopen(output_file.data(), "a");
    if(!f) {
        std::perror("File opening failed");
        return EXIT_FAILURE;
    }


    {
        // reading of the storage level 
        std::string file_storage;
        name_reading_blocks_rank(argv[1], file_storage);
        auto b_ranks = read_data_binary<uint64_t>(file_storage, false);
        int fd = open_read_file(argv[1]);

        size_t length_storage = (b_ranks.size()-1)*logical_block_size;
        std::cout<<"length_storage "<<length_storage<<std::endl;

        //create the mapping to read the strings
        mmap_content.open_mapping(fd, 0, length_storage);
        char * mmapped_content = mmap_content.get_mmapped_content();
        
        //get the first string of each logical block
        for(size_t i=0; i<b_ranks.size()-1; i++){ //the last value is associated at the end of the last block 
            std::string s(mmapped_content + i * logical_block_size);
            fprintf(f,"%s\n", s.data());
        }
            
        mmap_content.close_mapping(length_storage);
        close_file(fd);
    }

    //close the output file
    std::fclose(f);

    return 0;
}