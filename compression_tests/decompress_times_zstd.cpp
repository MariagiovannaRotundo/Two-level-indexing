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

#include "mmap_level_zstd.hpp"


#define SIZE_OUTPUT 100000


ZSTD_DStream* zstd_stream;
ZSTD_inBuffer inBuffer;
ZSTD_outBuffer outBuffer;

std::string output;

void zstd_decompress_block(size_t block, char * mmapped_content, size_t logical_block_size);



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


    std::string output_file("block-wise_decompression_stats");
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

        fprintf(f,"name_compressor,dataset,logical_b_size,avg_decomp_time,disk_space\n");// newline
    }
    else{
        f = std::fopen(output_file.data(), "a");
        if(!f) {
            std::perror("File opening failed");
            return EXIT_FAILURE;
        }
    }    


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
    
    //decompression on 1/10 of the total number of blocks
    while(blocks.size()<(num_blocks/10)){
        auto number = rand() % num_blocks;
        auto results = blocks.insert(number);
        if(results.second){
            unordered_blocks.push_back(number); //contain the number/index of the block to decompress
        }
    }

    // ********** decompress blocks compressed with ZSTD **********

    output.resize(SIZE_OUTPUT);
    
    zstd_stream = ZSTD_createDStream();
    if(zstd_stream == NULL){
        std::cout<<"error in the creation of the zstd stream"<<std::endl;
        exit(0);
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i=0; i < unordered_blocks.size(); i++){
        zstd_decompress_block(unordered_blocks[i], mmapped_content, logical_block_size);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    size_t free_stream = ZSTD_freeDStream(zstd_stream);

    std::cout << "\naverage time to decompress a block: "<<ns/unordered_blocks.size()<<std::endl;

    fprintf(f,"zstd,%s,%ld,%ld,%ld\n", argv[1], logical_block_size, ns/unordered_blocks.size(), length_storage);


    //close the mmap anf the files
    mmap_content.close_mapping(length_storage);
    close_file(fd);
    
    std::fclose(f);

    return 0;
}





void zstd_decompress_block(size_t block, char * mmapped_content, size_t logical_block_size){

    size_t size_uncompressed = 0;
    auto curr_str = output.data();

    auto data_ptr = mmapped_content + block * logical_block_size;
    auto first_string = data_ptr;
    auto first_len = strlen(data_ptr) + 1;
    data_ptr += first_len;

    //pointer to the last bit of the block
    auto data_ptr_end = mmapped_content + (block+1) * logical_block_size -1;
    int padding = 0;
    while(data_ptr_end[0]==0 && (data_ptr_end - data_ptr) >=0){ 
        data_ptr_end -= 1;
        padding++;
    }

    if(data_ptr - data_ptr_end <= 0){
        //in the block there is only one string and it is not compressed (because it is the first one): stop
        return;
    }

    //if in the block there are more than 1 string, start the decompression.

    size_t reset_stream = ZSTD_DCtx_reset(zstd_stream, ZSTD_reset_session_and_parameters); //to reset session and parameters
    size_t init_stream = ZSTD_initDStream(zstd_stream); //the compression level is 12
    if(ZSTD_isError(init_stream)){
        std::cout<<"init error "<<ZSTD_getErrorName(init_stream)<<std::endl;
    }
    size_t load_dict_stream = ZSTD_DCtx_loadDictionary(zstd_stream, first_string, first_len);

    inBuffer.src = data_ptr;
    inBuffer.size = data_ptr_end - data_ptr +1; 
    inBuffer.pos = 0; /**< position where reading stopped. Will be updated. Necessarily 0 <= pos <= size */


    outBuffer.dst = output.data(); /**< start of output buffer */
    outBuffer.size = SIZE_OUTPUT; /**< size of output buffer */
    outBuffer.pos = 0;


    size_t res;
    do{
        res = ZSTD_decompressStream(zstd_stream, &outBuffer, &inBuffer); 
    }while(!ZSTD_isError(res) && res!=0);

    if(ZSTD_isError(res)){
        std::cout<<"Decompression error: "<<ZSTD_getErrorName(res)<<std::endl;
        std::cout<<"Src size "<<inBuffer.size<<std::endl;
        std::cout<<"first string length "<<first_len<<std::endl;
        exit(0);
    }

    auto size_decompressed = outBuffer.pos;

    if(size_decompressed==0){
        std::cout<<"size compressed equal to 0: "<<std::endl;
        exit(0);
    }
    
}
