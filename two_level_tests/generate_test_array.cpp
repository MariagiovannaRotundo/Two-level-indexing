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

#define REPEAT 3



//read strings from file
// - input: file to which read the strings; vector of strings where to load them
int read_strings_from_file(char* filename, std::vector<std::string>& wordList){

    {
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
    }

    return 0;
}


// return the space used by the indexing layer and the data structure used to answer to rank opations
// - input: string where concatenate the string dictionary; the array where to store the pointers
size_t get_Size(std::string& data,  sdsl::int_vector<>& pointers,  sdsl::int_vector<>& blocks_rank){
    size_t size = 0;
    size += data.length() * sizeof(data[0]);
    size += sdsl::size_in_bytes(pointers);
    size += sdsl::size_in_bytes(blocks_rank);
    return size;
}

//compute the longest common prefix between the strings s1 and s2
// - input: the strings s1 and s2
int lcp(std::string s1, std::string s2){
    auto n = std::min(s1.length(), s2.length());
    for(int i=0; i<n; i++)
        if(s1[i] != s2[i])
            return i;
    return n;
}

//compute the distinguishing prefix of string s2 by looking at string s1 and s3
// - input: strings s1, s2, and s3
std::string trunc(std::string s1, std::string s2, std::string s3){
    auto m = std::max(lcp(s1, s2), lcp(s2, s3));
    m = std::min(s2.length(), (size_t)(m + 1));
    return s2.substr(0, m);
}


//given a set of strings, compute their distinguishing prefixes and concatenate them keeping pointers to the position
//where strings start
// - input: the vector of strings; the string where to concatenate the distinguishing prefix; the vector where store the pointers
void create_array_strings_distinguish(std::vector<std::string>& wordList, std::string& data, sdsl::int_vector<>& pointers){

    std::string word_pre = "";

    for(int i = 0; i< wordList.size()-1; i++){
        pointers[i] = data.size();
        data.append(trunc(word_pre, wordList[i],  wordList[i+1]));
        data.push_back('\0');

        word_pre = wordList[i];
    }
    //for the last string
    pointers[wordList.size()-1] = data.size();
    data.append(trunc(word_pre, wordList[wordList.size()-1],  ""));
    data.push_back('\0');

    sdsl::util::bit_compress(pointers);
    data.shrink_to_fit();
}


//look for a string s in the storage level returning its rank
// - input: vecotr of pointers to the strings; string that contain the concatenated distinguishin prefixes;
// the string to search; content of the mmap; size of the logical blocks; vector with the ranks of the blocks; 
size_t lookup_array_disting(sdsl::int_vector<>& pointers, std::string& data, std::string& query_word, 
    MmappedSpace& mmap_content, size_t logical_block_size, sdsl::int_vector<>& blocks_rank){


    //n is the number of logical blocks
    auto n = blocks_rank.size()-1;
    int i = 0;
    int j = n - 1;
    int m;
    int k = 0; //to iterate on strings

    //implementation of binary search
    while(i<=j){
        k = 0; //to start to scan strings from the first char
        m = floor((i + j) / 2);

        while(data[pointers[m]+k] != '\0' && query_word[k]!='\0' && data[pointers[m]+k] == query_word[k]){
            k++; //where the mismatch occurs
        }
        if(data[pointers[m]+k] == '\0' && data[pointers[m]+k] == query_word[k]){ //the string was found
		    return blocks_rank[m];
        }
        else{
            if((unsigned char) data[pointers[m]+k] < (unsigned char) query_word[k]){
                i = m+1;
            }else{
                j = m -1;
            }
        }                 
    }

    //access to the first string of the block (correspond to the string associated with the found distinguishing prefix)
	char * first = mmap_content.access_at((i-1) * logical_block_size);

	k = 0;
	while(first[k] != '\0' && query_word[k]!='\0' && first[k] == query_word[k]){
        k++; //where the mismatch occurs
    }
	if(first[k] == '\0' && first[k] == query_word[k]){
		return blocks_rank[i-1];
    }
	
	if((unsigned char) first[k] < (unsigned char) query_word[k]){
        //scan the current block
        return mmap_content.scan_block(query_word, (i-1) * logical_block_size, blocks_rank[i] - blocks_rank[i-1], blocks_rank[i-1]);
    }else{
        //scan the previous block
        return mmap_content.scan_block(query_word, (i-2) * logical_block_size, blocks_rank[i-1] - blocks_rank[i-2], blocks_rank[i-2]);
    }

}



//ecute test of the two-level approach based on Array of pointers for the indexing level
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

    //compute the size ofthe logical blocks
    size_t apprx_logical_b_size = atoi(argv[2])<<10; // *1024 bytes    
    // https://man7.org/linux/man-pages/man3/sysconf.3.html
    auto page_size = sysconf(_SC_PAGE_SIZE);
    size_t logical_block_size = (apprx_logical_b_size & ~(page_size - 1)) + (apprx_logical_b_size%page_size!=0)*page_size;


    MmappedSpace mmap_content;
    size_t us;

    //open the output file
    std::string output_file(argv[4]);
    output_file.append(".csv");

    const std::filesystem::path fresult{output_file}; 
    FILE* f;
    if(!std::filesystem::exists(fresult)){ 
        f = std::fopen(output_file.data(), "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }
        fprintf(f,"name,dataset_trie,logical_b_size,avg_time,space,disk_space,construction_time\n");// newline
    }
    else{
        f = std::fopen(output_file.data(), "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }
    }    


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

    //string where to concatenate the distinguishing prefixes
    std::string data;
    data.reserve(1 << 22);

    sdsl::int_vector<> blocks_rank(b_ranks.size());

    //read the first string of each block
    std::vector<std::string> memory_strings;
    for(size_t i=0; i<b_ranks.size()-1; i++){ //the last value 
        std::string s(mmapped_content+ i * logical_block_size);
        memory_strings.push_back(s);
    }
    sdsl::int_vector<> pointers(memory_strings.size());

    //create the indexing layer
    auto t0_build = std::chrono::high_resolution_clock::now();
    create_array_strings_distinguish(memory_strings, data, pointers);
    auto t1_build = std::chrono::high_resolution_clock::now();
    us = std::chrono::duration_cast<std::chrono::microseconds>(t1_build - t0_build).count();

    std::cout << "built the array in " << us << " us\n" << std::endl;  
    
    for(size_t i=0; i<b_ranks.size(); i++){
        blocks_rank[i] = b_ranks[i];
    }

    b_ranks.clear();
    b_ranks.shrink_to_fit();

    memory_strings.clear();
    memory_strings.shrink_to_fit();
    

    //get the space used by the indexing level
    size_t size = get_Size(data, pointers, blocks_rank);  
    std::cout << "size in internal memory " << size << std::endl;
    std::cout << "size on disk " << length_storage << std::endl;
    std::cout << "total size " << size +  length_storage << std::endl;

    //query

    size_t rank = 0;

    //read from file the strings to use as query
    std::vector<std::string> query_word;
    read_strings_from_file(argv[3], query_word);

    auto t0 = std::chrono::high_resolution_clock::now();
    for(size_t i=0; i< repetitions; i++){
        for(size_t i=0; i< query_word.size(); i++){
            
            rank = lookup_array_disting(pointers, data, query_word[i], mmap_content, logical_block_size, blocks_rank);

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

    std::cout << "\naverage time for the lookup with array solution: "<<ns/(query_word.size()*repetitions)<<std::endl;

    fprintf(f,"Array,%s,%ld,%ld,%ld,%ld,%ld\n", argv[1], logical_block_size, ns/(query_word.size()*repetitions), size, length_storage, us);

    //close the mmap anf the files
    mmap_content.close_mapping(length_storage);
    close_file(fd);
    
    std::fclose(f);

    return 0;
}

