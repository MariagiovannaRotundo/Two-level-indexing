#include <string>
#include <fstream>
#include <vector>
#include <string.h>
#include <filesystem>
#include <cstdio>
#include <sdsl/vectors.hpp>

#include "common_methods.hpp"


#define PERCENTAGE 10
#define REPEAT 3


//concatenate the input strings and create the array of pointers
// - input: the vector of strings to concatenate; the string where to concatenate the strings; the array of pointers
void create_array_strings(std::vector<std::string>& wordList, std::string& data, uint * pointers){

    for(int i = 0; i< wordList.size(); i++){
        pointers[i] = data.size();
        data.append(wordList[i]);
        data.push_back('\0');
    }

    data.shrink_to_fit();
}



//simulate the binary search on the array of pointers by forcing the recursion
// - input: the array of pointers; the string that contain the concatenation of the strings;
// the string to search; the number of strings concatenated
int lookup_array(uint * pointers, std::string& data, std::string& query_word, int n){

    int i = 0;
    int j = n - 1;
    int m;
    int rank = i;
    int k = 0; 

    while(i<=j){
        k = 0;
        m = floor((i + j) / 2);

        char * data_p = &data[pointers[m]];

        while(data[pointers[m]+k] != '\0' && query_word[k]!='\0' && data[pointers[m]+k] == query_word[k]){
            k++; //where the mismatch occurs
        }
        if(data[pointers[m]+k] < query_word[k]){
            i = m+1;
            rank = m;
        }else{ //data[pointers[m]+k] >= query_word[k]
            j = m -1;
        }           
    }
    return i;
}



//test the array of string pointers solution
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
        std::string data;
        data.resize(1 << 22);
        uint * pointers = (uint *) malloc(sizeof(uint) * word_block.size()); // a pointer for each string    

        // create the array of pointers
        auto t0_build = std::chrono::high_resolution_clock::now();
        create_array_strings(word_block, data, pointers);
        auto t1_build = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1_build - t0_build).count();
        std::cout << "\nbuilt the array of pointers in " << us << " us" << std::endl;    

        //size
        size_t size = sizeof(uint) * word_block.size() + data.size() * sizeof(data[0]); //array of pointers + strings  

        std::cout << "\nsize of pointers " <<  sizeof(uint) * word_block.size() << std::endl;
        std::cout << "size of strings " << data.size() * sizeof(data[0]) << std::endl;
        std::cout << "total size " << size << std::endl;

        //lookup

        size_t rank = 0;

        auto t0 = std::chrono::high_resolution_clock::now();
        for(int i=0; i< REPEAT; i++){
            for(int i=0; i< query_word.size(); i++){
                rank = lookup_array(pointers, data, query_word[i], word_block.size());
            }
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

        volatile auto temp = rank;

        std::cout << "\naverage time for the lookup with array: "<<ns/(query_word.size()*REPEAT)<<std::endl;

        //get the name of the input dataset
        std::string input_name(argv[1]);
        int pos = input_name.find_last_of('/');
        std::string name_in = input_name.substr(pos+1); 

        //output file
        const std::filesystem::path fresult{"./array_results.csv"}; //array_on_sampling.csv
        if(!std::filesystem::exists(fresult)){ //if the file fo not exists
            FILE* f = std::fopen("./array_results.csv", "a");
            if(!f) {
                std::perror("File opening failed");
                return EXIT_FAILURE;
            }

            fprintf(f,"name,dataset_trie,block_size,avg_time,space,construction_time\n");// newline
            fprintf(f,"%s,%s,%d,%ld,%ld,%ld\n","Array", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), size,us);
            std::fclose(f);
        }
        else{
            FILE* f = std::fopen("./array_results.csv", "a");
            if(!f) {
                std::perror("File opening failed");
                return EXIT_FAILURE;
            }
            fprintf(f,"%s,%s,%d,%ld,%ld,%ld\n","Array", name_in.data(), blockSize, ns/(query_word.size()*REPEAT), size,us);
            std::fclose(f);
        }
    }
    
    return 0;
}


