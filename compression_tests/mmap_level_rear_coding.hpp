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
#include <sdsl/vectors.hpp>

#include "storage_level_rear_coding.hpp"



class MmappedSpace {        // The class
  public:              // Access specifier

    char * mmapped_content;

    //mmap the file referred by the file descriptor fd starting from offset "start_offset" for the specifyed length
    // - MMAP open in READ mode (not write allowed)
    // - input: the file descriptor of file to mmap; offset where starts the mapping; length of the mapping
    void open_mapping(int fd, size_t start_offset, size_t length){
        mmapped_content = (char *) mmap(NULL, length, PROT_READ, MAP_SHARED, fd, start_offset);
        if(mmapped_content == MAP_FAILED){
            std::cout<<"ERROR IN THE MMAPPING OF THE FILE"<<std::endl;
            std::cout << "mmapping failure: " << std::strerror(errno) << '\n';
            exit(0);
        }
    }  

    //return the content of the mapping
    char * get_mmapped_content(){
        return mmapped_content;
    }

    //close the mmap previously opened with specified length 
    // - input: length of the mmap
    void close_mapping(size_t length){
        auto unmmapped = munmap(mmapped_content, length);
        if(unmmapped == -1){
            std::cout<<"ERROR IN THE MUNMAP OF THE FILE"<<std::endl;
            std::cout << "munmap failure: " << std::strerror(errno) << '\n';
            exit(0);
        }
    }

};
