#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <iterator>

#include <zstd.h>

#define SIZE_OUTPUT 100000
#define THRESHOLD 3

//open a file in reading and writing with the open() system call
// - input: string with name of the file to open
int open_file(std::string name_file){

    //create the file if it does not exist
    FILE* f = std::fopen(name_file.data(), "w");
    if(!f) {
        std::perror("File opening failed");
        exit(0);
    }
    std::fclose(f);

    // https://linux.die.net/man/3/open
    int fd = open(name_file.data(), O_RDWR);
    if(fd == -1){
        std::cout<<"ERROR IN THE OPENING OF THE FILE"<<std::endl;
        std::cout << "opening failure: " << std::strerror(errno) << '\n';
        exit(0);
    }
    std::cout << "file opened " << std::endl;

    return fd;
}


//open a file in read-only mode with the open() system call
// - input: string with name of the file to open
int open_read_file(std::string name_file){

    // https://linux.die.net/man/3/open
    int fd = open(name_file.data(), O_RDONLY); 
    if(fd == -1){
        std::cout<<"ERROR IN THE OPENING OF THE FILE"<<std::endl;
        std::cout << "opening failure: " << std::strerror(errno) << '\n';
        exit(0);
    }
    std::cout << "file opened " << std::endl;

    return fd;
}


//allocate disk space for the file referred by the file descriptor "fd" starting from
//"offset" and for the length "compressed_space"
// - input: the file descriprot fo the file to which allocate space; the offset from which 
//starting the allocation; the length in bytes to allocate
void allocate_file(int fd, size_t offset, size_t compressed_space){
    // https://man7.org/linux/man-pages/man2/fallocate.2.html - nonportable, Linux-specific system call
    // int alloc = fallocate(fd, 0, offset, compressed_space); //Allocating disk space (mode = 0)
    //https://man7.org/linux/man-pages/man3/posix_fallocate.3.html - portable
    int alloc = posix_fallocate(fd, offset, compressed_space); 
    if(alloc){
        std::cout<<"ERROR IN THE ALLOCATION OF THE FILE: "<<alloc<<std::endl;
        // std::cout << "allocation failure: " << std::strerror(errno) << '\n';
        exit(0);
    }
    std::cout << "space allocated " << std::endl;
}



//deallocate the file descriptor "fd" with the system call close()\
// - input: the file descriptor
void close_file(int fd){
    // https://linux.die.net/man/3/close
    int closed = close(fd);
    if(closed == -1){
        std::cout<<"ERROR IN THE CLOSING OF THE FILE"<<std::endl;
        std::cout << "opening failure: " << std::strerror(errno) << '\n';
        exit(0);
    }
    std::cout << "file closed " << std::endl;
}


//compute the LCP of the two strings "a" and "b"
// - input: the two strings for which compute the LCP
size_t compute_lcp(std::string_view a, std::string_view b) {
    return std::mismatch(a.begin(), a.end(), b.begin(), b.end()).first - a.begin();
}

//compute the LCP of the two strings "a" and "b"
// - input: the char * that points to the 2 strings
size_t compute_lcp(const char *a, const char *b) {
    size_t i = 0;
    while (a[i] != '\0' && a[i] == b[i])
        ++i;
    return i;
}

//encode the numerical (positive) value "x" in the string "out" by Variable-byte coding
// - input: the numerica value >=0; the string where encode the value
static void encode_int(size_t x, std::string &out) {
    while (x > 127) {
        out.push_back(uint8_t(x) & 127);
        x >>= 7;
    }
    out.push_back(uint8_t(x) | 128);
}

//decode a numerical (positive) value "x" encoded as characters in the string "out" by Variable-byte coding
// - input: the pointer to the characters to decode
static size_t decode_int(char *&in) {
    size_t result = 0;
    uint8_t shift = 0;
    while (*in >= 0) {
        result |= size_t(*in) << shift;
        ++in;
        shift += 7;
    }
    result |= size_t(*in & 127) << shift;
    ++in;
    return result;
}


//APPLY Zstd TO THE BLOCK BY USING THE FIRST STRING (UNCOMPRESSED) AS DICTIONARY
// - logical_block_size is expressed as a multiple of the physical size of blocks (that can be retrieved with 
//sysconf(_SC_PAGE_SIZE) system call)
// - input: a char * to the name of file where strings are stored; the logical block size
size_t compute_disk_space_zstd_look_first(char* filename, int logical_block_size){

    std::string word;
    std::string prev_word;
    bool first_s = true;
    size_t size = 0;
    std::string block_first;
    std::string block_compressed_string;

    std::string output;
    output.resize(SIZE_OUTPUT);

    ZSTD_inBuffer inBuffer;
    ZSTD_outBuffer outBuffer;

    ZSTD_inBuffer inBuffer_pre;
    ZSTD_outBuffer outBuffer_pre;

    std::string tmp_block;
    std::string tmp_block_pre;

    ZSTD_CStream* zstd_stream = ZSTD_createCStream();
    if(zstd_stream == NULL){
        std::cout<<"error in the creation of the zstd stream"<<std::endl;
        exit(0);
    }

    ZSTD_CStream* zstd_stream_pre = ZSTD_createCStream();
    if(zstd_stream_pre == NULL){
        std::cout<<"error in the creation of the zstd stream"<<std::endl;
        exit(0);
    }

    //oper the file in read mode
    std::ifstream ifs(filename, std::ifstream::in); 
    if (!ifs){
        std::cout<<"cannot open "<<filename<<"\n"<<std::endl;
        return -1;
    }

    //read the strings of the file one at a time
    while (getline(ifs, word)){
        //skip empty words
        if (word == "") continue;
    
        if(first_s){
            //create the first block of the storage by considering the first string
            first_s = false;
            //+1 for the '\0' added at the end of the string (no bytes is considered for the rear coding since its the first string)
            block_first = word; //since a new block starts
            block_first.push_back('\0');

            block_compressed_string = "";
        
            //note : since v1.3.0, ZSTD_CStream and ZSTD_CCtx are the same thing.
            size_t reset_stream = ZSTD_CCtx_reset(zstd_stream, ZSTD_reset_session_and_parameters); //3 to reset session and parameters
            size_t init_stream = ZSTD_initCStream(zstd_stream, 12); //12 is the compression level
            
            if(ZSTD_isError(init_stream)){
                std::cout<<"init error "<<ZSTD_getErrorName(init_stream)<<std::endl;
            }

            size_t load_dict_stream = ZSTD_CCtx_loadDictionary(zstd_stream, block_first.data(), block_first.size());

            size_t reset_stream_pre = ZSTD_CCtx_reset(zstd_stream_pre, ZSTD_reset_session_and_parameters); //3 to reset session and parameters
            size_t init_stream_pre = ZSTD_initCStream(zstd_stream_pre, 12); //12 is the compression level
            size_t load_dict_stream_pre = ZSTD_CCtx_loadDictionary(zstd_stream_pre, block_first.data(), block_first.size());
        }
        else{
            
            //compress the new string looking at the others
            tmp_block = word;
            tmp_block.push_back('\0'); 

            inBuffer.src = tmp_block.data();
            inBuffer.size = tmp_block.size();
            inBuffer.pos = 0; /**< position where reading stopped. Will be updated. Necessarily 0 <= pos <= size */


            outBuffer.dst = output.data(); /**< start of output buffer */
            outBuffer.size = SIZE_OUTPUT; /**< size of output buffer */
            outBuffer.pos = 0; /**< position where writing stopped. Will be updated. Necessarily 0 <= pos <= size */

            //COMPRESS   
            size_t res;
            do{
                res = ZSTD_compressStream2(zstd_stream, &outBuffer, &inBuffer, ZSTD_e_continue); 
            }while(!ZSTD_isError(res) && inBuffer.pos<inBuffer.size);

            if(ZSTD_isError(res)){
                std::cout<<"Compression error: "<<ZSTD_getErrorName(res)<<std::endl;
                exit(0);
            }

            //COMPRESS WITH FLUSH
            do{
                res = ZSTD_compressStream2(zstd_stream, &outBuffer, &inBuffer, ZSTD_e_flush); 
            }while(!ZSTD_isError(res) && res!=0);

            if(ZSTD_isError(res)){
                std::cout<<"Compression flush error: "<<ZSTD_getErrorName(res)<<std::endl;
                exit(0);
            }

            auto size_compressed = outBuffer.pos;

            if(size_compressed==0){
                std::cout<<"size compressed equal to 0: "<<std::endl;
                exit(0);
            }
                
            //+1 for the '\0' at the end of the new compressed string
            if(block_first.size() + block_compressed_string.size() + size_compressed > logical_block_size - THRESHOLD){
                //create a new full block adding its size to the total size of the storage 
                size += logical_block_size;
                //the size of the strings inserted in the current block is the uncompressed length of the string + the \0

                if(block_compressed_string.size() != 0){ //il blocco è composto solo dalla prima stringa perchè aggiungendo anche
                //una stringa compressa si sfora la dimensione del blocco

                    size_t pre_size = outBuffer_pre.pos;

                    //COMPRESS TO END: ADD AND EPILOGUE NEEDED FOR DECOMPRESSION
                    size_t res_pre;
                    do{
                        res_pre = ZSTD_compressStream2(zstd_stream_pre, &outBuffer_pre, &inBuffer_pre, ZSTD_e_end); //2 = end
                    }while(!ZSTD_isError(res_pre) && res_pre!=0);


                    if(ZSTD_isError(res_pre)){
                        std::cout<<"Compression end error: "<<ZSTD_getErrorName(res_pre)<<std::endl;
                        exit(0);
                    }

                    if(block_first.size() + block_compressed_string.size() + outBuffer_pre.pos - pre_size > logical_block_size){
                        std::cout<<"too many data: not fitting in the block "<<std::endl;
                        exit(0);
                    }

                }

                block_first = word; //since a new block starts
                block_first.push_back('\0');

                size_t reset_stream = ZSTD_CCtx_reset(zstd_stream, ZSTD_reset_session_and_parameters); //3 to reset session and parameters
                size_t init_stream = ZSTD_initCStream(zstd_stream, 12); //12 is the compression level
                size_t load_dict_stream = ZSTD_CCtx_loadDictionary(zstd_stream, block_first.data(), block_first.size());


                size_t reset_stream_pre = ZSTD_CCtx_reset(zstd_stream_pre, ZSTD_reset_session_and_parameters); //3 to reset session and parameters
                size_t init_stream_pre = ZSTD_initCStream(zstd_stream_pre, 12); //12 is the compression level
                size_t load_dict_stream_pre = ZSTD_CCtx_loadDictionary(zstd_stream_pre, block_first.data(), block_first.size());

                block_compressed_string = "";

            }
            else{ 

                tmp_block_pre = word;
                tmp_block_pre.push_back('\0'); 

                inBuffer_pre.src = tmp_block_pre.data();
                inBuffer_pre.size = tmp_block_pre.size();
                inBuffer_pre.pos = 0; /**< position where reading stopped. Will be updated. Necessarily 0 <= pos <= size */


                outBuffer_pre.dst = output.data(); /**< start of output buffer */
                outBuffer_pre.size = SIZE_OUTPUT; /**< size of output buffer */
                outBuffer_pre.pos = 0; /**< position where writing stopped. Will be updated. Necessarily 0 <= pos <= size */
               
                size_t res_pre;
                do{
                    res_pre = ZSTD_compressStream2(zstd_stream_pre, &outBuffer_pre, &inBuffer_pre, ZSTD_e_continue); //0 = ZSTD_e_continue
                }while(!ZSTD_isError(res_pre) && inBuffer.pos<inBuffer.size);

                if(ZSTD_isError(res_pre)){
                    std::cout<<"Compression error: "<<ZSTD_getErrorName(res_pre)<<std::endl;
                    exit(0);
                }


                do{
                    res_pre = ZSTD_compressStream2(zstd_stream_pre, &outBuffer_pre, &inBuffer_pre, ZSTD_e_flush); //1 = flush
                }while(!ZSTD_isError(res_pre) && res_pre!=0);

                if(ZSTD_isError(res_pre)){
                    std::cout<<"Compression error: "<<ZSTD_getErrorName(res_pre)<<std::endl;
                    exit(0);
                }

                block_compressed_string.append(output.data(), size_compressed); //+1 for the '\0'
            }
        }
        prev_word = word;
    }

    size_t free_stream = ZSTD_freeCStream(zstd_stream);
    size_t free_stream_pre = ZSTD_freeCStream(zstd_stream_pre);

    //add the size last block (possibly not full by still padded to reach size logical_block_size)
    size += logical_block_size;

    return size;

}






//compute the name of the file where writing the blockwise storage by considering block of size "logical_block_size". 
// - It will have form: "name_original_file_B_logical_block_size.txt"
// - input: char * to the name of file, string where to write the name of the output file; logical block size
void name_writing_file(char* input_file, std::string& output_file, int logical_block_size){
    std::string input(input_file);
    int pos_start = input.find_last_of('/');
    int pos_end = input.find_last_of('.');
    output_file = input.substr(pos_start+1, pos_end - pos_start-1); 
    output_file += "_B_"+ std::to_string(logical_block_size) +"_zstd.txt";
}



//APPLY Zstd TO THE WHOLE BLOCK USING THE FIRST STRING (LEFT UNCOMPRESSED) AS DICTIONARY
//create the storage composed of blocks of string with fixed length compressed with zstd and write this storage 
//on file referred by fd
// - input: file from which read the (uncompressed) dictionary of strings; the logical block size; the file descriptor of the 
//file where to write the storage; the vector where write blocks rank that for each block store the number of strings 
//that preceed it 
void write_compressed_strings_zstd_look_first(char* filename, int logical_block_size, int fd, std::vector<size_t>& blocks_rank){ //, std::vector<size_t>& off_logical_block

    std::string word;
    std::string prev_word;
    size_t n_strings = 0;
    std::string str_first_block;
    std::string str_logical_block;
    size_t busy_bytes = 0;

    std::string output;
    output.resize(SIZE_OUTPUT);

    ZSTD_inBuffer inBuffer;
    ZSTD_outBuffer outBuffer;

    ZSTD_inBuffer inBuffer_pre;
    ZSTD_outBuffer outBuffer_pre;

    ZSTD_CStream* zstd_stream = ZSTD_createCStream();
    if(zstd_stream == NULL){
        std::cout<<"error in the creation of the zstd stream"<<std::endl;
        exit(0);
    }

    ZSTD_CStream* zstd_stream_pre = ZSTD_createCStream();
    if(zstd_stream_pre == NULL){
        std::cout<<"error in the creation of the zstd stream"<<std::endl;
        exit(0);
    }


    //open the file to which read the string
    std::ifstream ifs(filename, std::ifstream::in); //open in reading
    if (!ifs){
        std::cout<<"cannot open "<<filename<<"\n"<<std::endl;
        exit(0);
    }

    //read the uncompressed strings
    while (getline(ifs, word)){
        //skip the empty strings
        if (word == "") continue;

        //if the current string is the first one create a new block
        if(n_strings == 0){
            str_first_block = word;
            str_first_block.push_back('\0');
            str_logical_block = "";

            //before the first block there are no strings (0)
            blocks_rank.push_back(n_strings);

            //note : since v1.3.0, ZSTD_CStream and ZSTD_CCtx are the same thing.
            size_t reset_stream = ZSTD_CCtx_reset(zstd_stream, ZSTD_reset_session_and_parameters); //3 to reset session and parameters
            size_t init_stream = ZSTD_initCStream(zstd_stream, 12); //12 is the compression level
           
            if(ZSTD_isError(init_stream)){
                std::cout<<"init error "<<ZSTD_getErrorName(init_stream)<<std::endl;
            }
            size_t load_dict_stream = ZSTD_CCtx_loadDictionary(zstd_stream, str_first_block.data(), str_first_block.size());

            size_t reset_stream_pre = ZSTD_CCtx_reset(zstd_stream_pre, ZSTD_reset_session_and_parameters); //3 to reset session and parameters
            size_t init_stream_pre = ZSTD_initCStream(zstd_stream_pre, 12); //12 is the compression level
            size_t load_dict_stream_pre = ZSTD_CCtx_loadDictionary(zstd_stream_pre, str_first_block.data(), str_first_block.size());

        }
        else{

            //compress the new string looking at the others
            std::string tmp_block = word;
            tmp_block.push_back('\0'); 

            inBuffer.src = tmp_block.data();
            inBuffer.size = tmp_block.size();
            inBuffer.pos = 0; /**< position where reading stopped. Will be updated. Necessarily 0 <= pos <= size */

            outBuffer.dst = output.data(); /**< start of output buffer */
            outBuffer.size = SIZE_OUTPUT; /**< size of output buffer */
            outBuffer.pos = 0; /**< position where writing stopped. Will be updated. Necessarily 0 <= pos <= size */

            //COMPRESS   
            size_t res;
            do{
                res = ZSTD_compressStream2(zstd_stream, &outBuffer, &inBuffer, ZSTD_e_continue); 
            }while(!ZSTD_isError(res) && inBuffer.pos<inBuffer.size);

            if(ZSTD_isError(res)){
                std::cout<<"Compression error: "<<ZSTD_getErrorName(res)<<std::endl;
                exit(0);
            }

            //COMPRESS WITH FLUSH
            do{
                res = ZSTD_compressStream2(zstd_stream, &outBuffer, &inBuffer, ZSTD_e_flush); 
            }while(!ZSTD_isError(res) && res!=0);

            if(ZSTD_isError(res)){
                std::cout<<"Compression flush error: "<<ZSTD_getErrorName(res)<<std::endl;
                exit(0);
            }

            auto size_compressed = outBuffer.pos;

            if(size_compressed==0){
                std::cout<<"size compressed equal to 0: "<<std::endl;
                exit(0);
            }
            

            //+1 for the '\0' at the end of the new compressed string
            if(str_first_block.size() + str_logical_block.size() + size_compressed > logical_block_size - THRESHOLD){
                //create a new full block adding its size to the total size of the storage 
            
                if(str_logical_block.size() != 0){

                    size_t pre_size = outBuffer_pre.pos;

                    //COMPRESS TO END: ADD AND EPILOGUE NEEDED FOR DECOMPRESSION
                    size_t res_pre;
                    do{
                        res_pre = ZSTD_compressStream2(zstd_stream_pre, &outBuffer_pre, &inBuffer_pre, ZSTD_e_end); //2 = end
                    }while(!ZSTD_isError(res_pre) && res_pre!=0);

                    if(ZSTD_isError(res_pre)){
                        std::cout<<"Compression end error: "<<ZSTD_getErrorName(res_pre)<<std::endl;
                        exit(0);
                    }

                    if(str_first_block.size() + str_logical_block.size() + outBuffer_pre.pos - pre_size > logical_block_size){
                        std::cout<<"too many data: not fitting in the block "<<std::endl;
                        exit(0);
                    }

                    str_first_block.append(str_logical_block);
                    str_first_block.append(output.data(), res_pre, outBuffer_pre.pos - pre_size); //append the bits at the end (ZSTD_e_end)
                
                }

                //the size of the strings inserted in the current block is the uncompressed length of the string + the \0
                char * writing_data = str_first_block.data();
                size_t writing_length = str_first_block.length();
                //indicate the offset of the file where starts the writing and correspond at the end of the last written block
                size_t writing_offset = busy_bytes;

                int wrote;
                do{
                    //https://linux.die.net/man/2/pread
                    wrote = pwrite(fd, writing_data, writing_length, writing_offset);
                    writing_data += wrote;
                    writing_length -= wrote;
                    writing_offset += wrote;
                }while(wrote>0 && writing_length>0);

                if(wrote == -1){
                    std::cout<<"ERROR IN THE OPENING OF THE FILE"<<std::endl;
                    std::cout << "writing failure: " << std::strerror(errno) << '\n';
                    exit(0);
                }

                busy_bytes += logical_block_size;


                str_first_block = word; //since a new block starts
                str_first_block.push_back('\0');
                str_logical_block = "";

                size_t reset_stream = ZSTD_CCtx_reset(zstd_stream, ZSTD_reset_session_and_parameters); //3 to reset session and parameters
                size_t init_stream = ZSTD_initCStream(zstd_stream, 12); //12 is the compression level
                size_t load_dict_stream = ZSTD_CCtx_loadDictionary(zstd_stream, str_first_block.data(), str_first_block.size());

                size_t reset_stream_pre = ZSTD_CCtx_reset(zstd_stream_pre, ZSTD_reset_session_and_parameters); //3 to reset session and parameters
                size_t init_stream_pre = ZSTD_initCStream(zstd_stream_pre, 12); //12 is the compression level
                size_t load_dict_stream_pre = ZSTD_CCtx_loadDictionary(zstd_stream_pre, str_first_block.data(), str_first_block.size());

                blocks_rank.push_back(n_strings);
                
            }
            else{ 

                inBuffer_pre.src = tmp_block.data();
                inBuffer_pre.size = tmp_block.size();
                inBuffer_pre.pos = 0; /**< position where reading stopped. Will be updated. Necessarily 0 <= pos <= size */

                outBuffer_pre.dst = output.data(); /**< start of output buffer */
                outBuffer_pre.size = SIZE_OUTPUT; /**< size of output buffer */
                outBuffer_pre.pos = 0; /**< position where writing stopped. Will be updated. Necessarily 0 <= pos <= size */
                
                size_t res_pre;
                do{
                    res_pre = ZSTD_compressStream2(zstd_stream_pre, &outBuffer_pre, &inBuffer_pre, ZSTD_e_continue); //0 = ZSTD_e_continue
                }while(!ZSTD_isError(res_pre) && inBuffer.pos<inBuffer.size);

                if(ZSTD_isError(res_pre)){
                    std::cout<<"Compression error: "<<ZSTD_getErrorName(res_pre)<<std::endl;
                    exit(0);
                }

                do{
                    res_pre = ZSTD_compressStream2(zstd_stream_pre, &outBuffer_pre, &inBuffer_pre, ZSTD_e_flush); //1 = flush
                }while(!ZSTD_isError(res_pre) && res_pre!=0);

                if(ZSTD_isError(res_pre)){
                    std::cout<<"Compression error: "<<ZSTD_getErrorName(res_pre)<<std::endl;
                    exit(0);
                }

                str_logical_block.append(output.data(), size_compressed);
            }
        }

        prev_word = word;
        n_strings++;
    }

    str_first_block.append(str_logical_block);

    //manage the last block
    char * writing_data = str_first_block.data();
    size_t writing_length = str_first_block.length();
    size_t writing_offset = busy_bytes;
    
    int wrote;
    do{
        //https://linux.die.net/man/2/pread
        wrote = pwrite(fd, writing_data, writing_length, writing_offset);
        writing_data += wrote;
        writing_length -= wrote;
        writing_offset += wrote;
    }while(wrote>0 && writing_length>0);

    if(wrote == -1){
        std::cout<<"ERROR IN THE WRITING IN THE FILE"<<std::endl;
        std::cout << "writing failure: " << std::strerror(errno) << '\n';
        exit(0);
    }

    blocks_rank.push_back(n_strings);

    //https://man7.org/linux/man-pages/man2/fsync.2.html
    //flush data on disk
    int sync = fsync(fd);
    if(sync == -1){
        std::cout<<"ERROR IN THE FSYNC OF THE FILE"<<std::endl;
        std::cout << "fsync failure: " << std::strerror(errno) << '\n';
        exit(0);
    }

    size_t free_stream = ZSTD_freeCStream(zstd_stream);
    size_t free_stream_pre = ZSTD_freeCStream(zstd_stream_pre);

}



//create the storage with fixed blocks having the input file and the (fixed) size of blocks
// - input: the name of the file with the strings; the logical block size; the vector where to write the block ranks
//(i.e. how many strings preceed each block)
void create_storage(char * filename, int logical_block_size, std::vector<size_t>& blocks_rank){//, std::vector<size_t>& off_logical_block

    // 1 : compute the space needed to allocate to write the compressed strings
    size_t compressedSpace = compute_disk_space_zstd_look_first(filename, logical_block_size);
    std::cout << "disk space "<< compressedSpace << std::endl;

    // 2: find the name of the file where write
    std::string file_aligned;
    name_writing_file(filename, file_aligned, logical_block_size);

    // 3: open the file and allocate the needed space
    int fd = open_file(file_aligned);
    allocate_file(fd, 0, compressedSpace); 

    // 4: write the strings in the file
    write_compressed_strings_zstd_look_first(filename, logical_block_size, fd, blocks_rank);
    std::cout << "strings written on disk" << std::endl;

    // 5: close the file
    close_file(fd);

}



//write a vector on file
// - input: vector to write; file where to write; indication about if write also the size
template<typename Data>
void vector_to_file(const Data &data, const std::string &path, bool write_size = true) {
    using value_type = typename Data::value_type;
    std::ofstream file(path, std::ios::out | std::ofstream::binary);
    if (write_size) {
        auto s = data.size();
        file.write(reinterpret_cast<const char *>(&s), sizeof(value_type));
    }
    file.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(value_type));
}


//read a vector from file
// - input: file from reading; indication about if checking if data is sorted
template<typename T>
std::vector<T> read_data_binary(const std::string &filename, bool check_sorted = true) {
    std::vector<T> data;
    try {
        std::fstream in(filename, std::ios::in | std::ios::binary);
        in.exceptions(std::ios::failbit | std::ios::badbit);
        T size;
        in.read((char *) &size, sizeof(T));
        data.resize(size);
        in.read((char *) data.data(), size * sizeof(T));
    } catch (std::ios_base::failure &e) {
        std::cerr << "Could not read the file. " << e.what() << std::endl;
        exit(1);
    }
    if (check_sorted && !std::is_sorted(data.begin(), data.end())) {
        std::cerr << "Input data must be sorted." << std::endl;
        std::cerr << "Read: [";
        std::copy(data.begin(), std::min(data.end(), data.begin() + 10), std::ostream_iterator<T>(std::cerr, ", "));
        std::cout << "...]." << std::endl;
        exit(1);
    }
    return data;
}


//automatically generate the name of file where to write the block ranks
// - input: the name of file with the strings; the string where to write the name; the logical block size
void name_writing_blocks_rank(char* input_file, std::string& output_file, int logical_block_size){
    std::string input(input_file);
    int pos_start = input.find_last_of('/');
    int pos_end = input.find_last_of('.');
    output_file = input.substr(pos_start+1, pos_end - pos_start-1); 
    output_file += "_B_"+ std::to_string(logical_block_size) +"_blocksRank_zstd.txt";
}

//automatically generate the name of file where to are stored the block ranks
// - input: the name of file with the blockwise storage; the string where to write the name
void name_reading_blocks_rank(char* input_file, std::string& output_file){
    std::string input(input_file);
    int pos_end = input.find_last_of('_');
    output_file = input.substr(0, pos_end); 
    output_file += "_blocksRank_zstd.txt";
}


//write on disk the block ranks on file
// - input: the name of file with the strings; the vector of block ranks; the logical block size
void write_file_block_ranks(char* input_file, std::vector<size_t>& blocks_rank, int logical_block_size){
    std::string output_name;
    name_writing_blocks_rank(input_file, output_name, logical_block_size);
    vector_to_file(blocks_rank, output_name);
}
