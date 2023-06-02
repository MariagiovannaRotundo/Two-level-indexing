#include <string>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <set>
#include <algorithm>

#include "mmap_level_fixed_blocks.hpp"

#define MAXSIZEQUERY 10000000

//binary search to retrieve the index of the last block whose rank is smaller or equal to the searched one (s_rank))
// - input: the vector with the ranks of the blocks; the rank of the searched string
size_t low_binary_search(std::vector<size_t>& b_ranks, size_t s_rank){
    int i = 0;
    int j = b_ranks.size()-2; //because b_ranks has a value more that the number of blocks
    int m;
    int pos = 0;

    while (i <= j){
        m = floor((i + j) / 2);
        if (b_ranks[m] < s_rank){
            i = m + 1;
            pos = m;
        }
        else if (b_ranks[m] > s_rank)
                j = m - 1;
            else
                return m;
    }
    return pos;
}



//retrieve the string of the dictionary with the indicated rank
// - input:content of the mmap; vector with the block ranks; size of the logical blocks; rank of the string to search
std::string get_string(char * mmapped_content, std::vector<size_t>& b_ranks, size_t logical_block_size, size_t s_rank){

    //bnary search to identify the correct block where search for the string
    size_t block = low_binary_search(b_ranks, s_rank);


    auto s_block = mmapped_content + block*logical_block_size; //first string (uncompressed) of the block
    auto decoded_string = std::string(s_block);
    
    size_t count = b_ranks[block]; //rank of the block

    while(count<s_rank){
        s_block += strlen(s_block) + 1;
        auto rear_length = decode_int(s_block);
        decoded_string = decoded_string.substr(0, decoded_string.length() - rear_length);
        decoded_string.append(s_block);
        count++;
    }

    return decoded_string;

}


//generate num_queries queries randomly sampling strings from from the storage layer.
// - input: the file with the storage level; the approximated logical block size expressed in KB (es 4096 bytes -> 4KB);
// prefix of file where to write the queries (it is added "-query.txt"); 
// the (optional) number of queries to generate (By default num_queries is 10M)
int main(int argc, char* argv[]){
    

    if (argc != 4 && argc != 5){
        std::cout<<"main.cpp fileStorage logicalBlockSize[KB] outputFile [num_queries]"<<std::endl;
        return -1;
    }

    //compute the size of the logical blocks as multiple of the physical size
    size_t apprx_logical_b_size = atoi(argv[2])<<10;
    // https://man7.org/linux/man-pages/man3/sysconf.3.html
    auto page_size = sysconf(_SC_PAGE_SIZE);
    size_t logical_block_size = (apprx_logical_b_size & ~(page_size - 1)) + (apprx_logical_b_size%page_size!=0)*page_size;

    //number of queries to generate
    size_t num_queries;
    if(argc == 4)
        num_queries = MAXSIZEQUERY;
    else
        num_queries = atoi(argv[4]);

    MmappedSpace mmap_content;

    std::string output_file = std::string(argv[3]) + "-query.txt";

    //read the ranks associated with the blocks
    std::string blocks_rank;
    name_reading_blocks_rank(argv[1], blocks_rank);
    auto b_ranks = read_data_binary<uint64_t>(blocks_rank, false);

    //open the file with the storage level
    int fd = open_read_file(argv[1]);
    size_t length_storage = (b_ranks.size()-1)*logical_block_size;
    std::cout<<"length_storage "<<length_storage<<std::endl;

    //open the mmap to read the strings
    mmap_content.open_mapping(fd, 0, length_storage);
    char * mmapped_content = mmap_content.get_mmapped_content();

    //get the total numner of strings
    size_t num_strings = b_ranks.back();
    std::cout<<"num strings "<<num_strings<<std::endl;

    //randomly sampling of the strings
    std::set<size_t> words; //ordered index of chosen words
    std::vector<size_t> unordered_words;
    
    srand(time(NULL));
    
    while(words.size()<=num_queries){
        auto number = rand() % num_strings;
        auto results = words.insert(number);
        if(results.second){
            unordered_words.push_back(number);
        }
    }

    //write the strings in the file
    std::ofstream writefile (output_file);
    if (writefile.is_open()){
       
        for(int j=0; j < unordered_words.size() - 1; j++){
            writefile<<get_string(mmapped_content, b_ranks, logical_block_size, unordered_words[j]);
            writefile<<"\n";
        }

        writefile<<get_string(mmapped_content, b_ranks, logical_block_size, unordered_words[unordered_words.size()-1]);
        writefile.close();
    }
    else{
        std::cout<<"Error"<<std::endl;
        mmap_content.close_mapping(length_storage);
        return 0;
    }
    
    mmap_content.close_mapping(length_storage);
    return 0;
}