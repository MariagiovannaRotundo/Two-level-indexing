#include <string>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <set>
#include <algorithm>

#include "patricia_trie.hpp"

#define MAXSIZEQUERY 10000000
#define SIZE_ALPHABET 256


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
std::string get_string(char * mmapped_content, std::vector<size_t>& b_ranks, size_t logical_block_size, size_t s_rank, std::string &curr_query){

    //bnary search to identify the correct block where search for the string
    size_t block = low_binary_search(b_ranks, s_rank);

    auto s_block = mmapped_content + block*logical_block_size; //first string (uncompressed) of the block
    auto decoded_string = std::string(s_block);

    size_t count = b_ranks[block]; 

    while(count<s_rank){
        s_block += strlen(s_block) + 1;
        auto rear_length = decode_int(s_block);
        decoded_string = decoded_string.substr(0, decoded_string.length() - rear_length);
        decoded_string.append(s_block);
        count++;
    }

    curr_query = decoded_string;
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

    std::string output_file = std::string(argv[3]) + "_adversarial-query.txt";

    //read the ranks associated with the blocks
    std::string file_storage;
    name_reading_blocks_rank(argv[1], file_storage);
    auto b_ranks = read_data_binary<uint64_t>(file_storage, false);

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
    

    //patricia trie 
    LOUDS_PatriciaTrie louds_pt;

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
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "\nbuilt the trie in " << us << " us" << std::endl;    

        for(size_t i=0; i<b_ranks.size(); i++){
            blocks_rank[i] = b_ranks[i];
        }

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


    //setting variable for searching on the trie
    size_t rank;
    //stack used to the upward traversal
    std::vector<size_t> seen_arcs;
    seen_arcs.resize(height);

    size_t fault_page = 0; //in case of analysis with mincore function

    std::string curr_query;

    std::cout<<"open the file "<<std::endl;
    //write the strings in the file
    std::ofstream writefile (output_file);
    if (!writefile.is_open()){
        std::cout<<"Error"<<std::endl;
        mmap_content.close_mapping(length_storage);
        return 0;
    }


    //generate queries
    std::cout<<"start generation"<<std::endl;

    std::set<std::tuple<size_t, size_t, size_t>> wrong_words;

    while(wrong_words.size()<num_queries){

        auto number = rand() % num_strings;
        get_string(mmapped_content, b_ranks, logical_block_size, number, curr_query);

        auto pos_mutation = rand() % ((size_t) curr_query.length());
        char new_value = (char) (rand() % (SIZE_ALPHABET-32)) + 32;
        
        curr_query[pos_mutation] = new_value;

        size_t random_io = 0;

        rank = louds_pt.lookup_mincore(mmap_content, curr_query, louds_select_0, seen_arcs, blocks_rank, 
            logical_block_size, &fault_page, &random_io);

        if(rank!=size_t(-1)){
            continue;
        }

        if(random_io == 2){

            std::tuple<int, int, int> triple{number, pos_mutation, new_value};
            
            auto results = wrong_words.insert(triple);
            if(results.second){
                writefile<<curr_query;
                writefile<<"\n";
            }
            else{
                std::cout<<"generated a duplicate word -> continue"<<std::endl;
            }
        }
        
    }

    writefile.close();
    
    mmap_content.close_mapping(length_storage);
    return 0;
}
