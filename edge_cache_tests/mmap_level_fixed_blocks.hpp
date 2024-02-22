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

#include "storage_level_fill_blocks.hpp"



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

    //return the content of mmap at the specified offset
    // - input: offset to which read data
    char * access_at(size_t offset) const {
        return mmapped_content+offset;
    }



    //search a string by scanning a block that starts in the mapping at the specified offset knowing the 
    //number of strings to scan and the rank of the block
    // - input: the string to search in the block; the offset where the block starts in the mapped content;
    // the maximum number of strings to see during the scan; the number of strings that preceed the block
    size_t scan_block(const std::string &s, size_t  offset, int n_strings, size_t block_rank) const {

        auto data_ptr = mmapped_content + offset; //first (uncompressed) string of the block
        auto first_string = data_ptr;
        //LCP between the searched stringand the first of the block
        unsigned int lcp_s = compute_lcp(s.data(), data_ptr);

        data_ptr += strlen(data_ptr) + 1; //second (compressed (rear, data)) string 
        auto rear_length = decode_int(data_ptr); 
      
        //LCP between the first and the second string of the block
        unsigned int lcp = strlen(first_string) - rear_length;
        
        char * s_block = (char*) data_ptr; //suffix of the second string of the block

        int j;
        //scan of the block
        for (j = 1; j < n_strings-1; j++) {  //last string managed outside the  loop

            data_ptr += strlen(s_block) + 1; //+1 to include the \0
            rear_length = decode_int(data_ptr); 

            if(lcp < lcp_s){ //the string is not present in the block (rank: j + block_rank)
                return -1;
            }
            if(lcp > lcp_s){
                //go to the next string in the block
                lcp += strlen(s_block)  - rear_length; 
                s_block = (char*) data_ptr;
                continue;
            }
            
            //continue to scan the block
            int k = 0;
            while(s[lcp_s] == s_block[k] && s[lcp_s]!='\0' && s_block[k]!='\0'){
                lcp_s++;
                k++;
            }

            if(lcp_s == s.length()){
                if(s.length() ==  strlen(s_block) + lcp){
                    return j + block_rank + 1; //string found in the block
                }
                else{   
                    return -1;
                }
            }
            if((unsigned char)s[lcp_s] < (unsigned char)s_block[lcp_s-lcp]){
                return -1;
            }

            lcp += strlen(s_block)  - rear_length; 
            s_block = (char*) data_ptr;
        }

        if(lcp != lcp_s){ //the string is not present in the block
            return -1;
        }
        //continue to scan the block
        int k = 0;
        while(s[lcp_s] == s_block[k] && s[lcp_s]!='\0' && s_block[k]!='\0'){
            lcp_s++;
            k++;
        }

        if(lcp_s == s.length() && s.length() ==  strlen(s_block) + lcp){
            return j + block_rank + 1; //string found
        }

        return -1;
    }


    //search a string by scanning a block that starts in the mapping at the specified offset knowing the 
    //number of strings to scan, the rank of the block ang skipping the first string of the block
    // - input: the string to search in the block; the LCP between the first string of the block and tjhe searched one;
    // the length of the first string; the offset where the block starts in the mapped content;
    // the maximum number of strings to see during the scan; the number of strings that preceed the block
    size_t scan_block_no_first(const std::string &s, int lcp_s, int len_first_string, size_t offset, int n_strings, size_t block_rank) const {
        
        auto data_ptr = mmapped_content + offset + len_first_string +1; //second (compressed) string of the block
        auto rear_length = decode_int(data_ptr);
        //LCP between the first and second string of the block
        unsigned int lcp = len_first_string - rear_length;

        char * s_block = (char*) data_ptr;

        int j;
        for (j = 1; j < n_strings-1; j++) {  //last string managed outside the loop

            data_ptr += strlen(s_block) +1; //+1 to include the '\0'  
            rear_length = decode_int(data_ptr);

            if(lcp < lcp_s){ //the string is not present in the block
                return -1;
            }
            if(lcp > lcp_s){
                //go to the next string in the block
                lcp += strlen(s_block)  - rear_length; 
                s_block = (char*) data_ptr;
                continue;
            }
            
            //continue to scan the block
            int k = 0;
            while(s[lcp_s] == s_block[k] && s[lcp_s]!='\0' && s_block[k]!='\0'){
                lcp_s++;
                k++;
            }

            if(lcp_s == s.length()){
                if(s.length() ==  strlen(s_block) + lcp){
                    return j + block_rank + 1; //string found
                }
                else{   
                    return -1;
                }
            }
            if((unsigned char)s[lcp_s] < (unsigned char)s_block[lcp_s-lcp]){
                return -1;
            }

            lcp += strlen(s_block)  - rear_length; 
            s_block = (char*) data_ptr;
        }

        if(lcp != lcp_s){ //the string is not in the block
            return -1;
        }
        //continue to scan the block
        int k = 0;
        while(s[lcp_s] == s_block[k] && s[lcp_s]!='\0' && s_block[k]!='\0'){
            lcp_s++;
            k++;
        }

        if(lcp_s == s.length() && s.length() ==  strlen(s_block) + lcp){
            return j + block_rank + 1; //string found
        }

        return -1;
    }


    //search a string by scanning a block that starts in the mapping at the specified offset knowing the 
    //number of strings to scan, the rank of the block, and the (partial) LCP between the query string and the 
    //first string of the block
    // - input: the string to search in the block; the LCP between the query string and the first string of the block;
    // the offset where the block starts in the mapped content;
    // the maximum number of strings to see during the scan; the number of strings that preceed the block
    size_t scan_block_with_lcp(const std::string &s, int start_lcp, size_t  offset, int n_strings, size_t block_rank) const {

        auto data_ptr = mmapped_content + offset; //first (uncompressed) string of the block
        auto first_string = data_ptr;
        //LCP between the searched stringand the first of the block
        unsigned int lcp_s = start_lcp + compute_lcp(s.data() + start_lcp, data_ptr + start_lcp);

        data_ptr += strlen(data_ptr) + 1; //second (compressed (rear, data)) string 
        auto rear_length = decode_int(data_ptr); 
      
        //LCP between the first and the second string of the block
        unsigned int lcp = strlen(first_string) - rear_length;
        
        char * s_block = (char*) data_ptr; //suffix of the second string of the block

        int j;
        //scan of the block
        for (j = 1; j < n_strings-1; j++) {  //last string managed outside the  loop

            data_ptr += strlen(s_block) + 1; //+1 to include the \0
            rear_length = decode_int(data_ptr); 

            if(lcp < lcp_s){ //the string is not present in the block (rank: j + block_rank)
                return -1;
            }
            if(lcp > lcp_s){
                //go to the next string in the block
                lcp += strlen(s_block)  - rear_length; 
                s_block = (char*) data_ptr;
                continue;
            }
            
            //continue to scan the block
            int k = 0;
            while(s[lcp_s] == s_block[k] && s[lcp_s]!='\0' && s_block[k]!='\0'){
                lcp_s++;
                k++;
            }

            if(lcp_s == s.length()){
                if(s.length() ==  strlen(s_block) + lcp){
                    return j + block_rank + 1; //string found in the block
                }
                else{   
                    return -1;
                }
            }
            if((unsigned char)s[lcp_s] < (unsigned char)s_block[lcp_s-lcp]){
                return -1;
            }

            lcp += strlen(s_block)  - rear_length; 
            s_block = (char*) data_ptr;
        }

        if(lcp != lcp_s){ //the string is not present in the block
            return -1;
        }
        //continue to scan the block
        int k = 0;
        while(s[lcp_s] == s_block[k] && s[lcp_s]!='\0' && s_block[k]!='\0'){
            lcp_s++;
            k++;
        }

        if(lcp_s == s.length() && s.length() ==  strlen(s_block) + lcp){
            return j + block_rank + 1; //string found
        }

        return -1;
    }

};
