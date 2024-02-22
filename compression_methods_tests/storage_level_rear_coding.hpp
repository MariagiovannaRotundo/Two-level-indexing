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
        std::cout<<"ERROR IN THE ALLOCATION OF THE FILE"<<std::endl;
        std::cout << "allocation failure: " << std::strerror(errno) << '\n';
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


//compute the space needed to store the dictionary of strings stored in file "filename" considering blocks 
//with fixed size "logical_block_size". In each block strings are compressed with rear coding and in each block are inserted
//as many strings as possibile to reach (but not exceed) the logical_block_size.
// - logical_block_size is expressed as a multiple of the physical size of blocks (that can be retrieved with 
//sysconf(_SC_PAGE_SIZE) system call)
// - input: a char * to the name of file where strings are stored; the logical block size
size_t compute_disk_space(char* filename, int logical_block_size){

    std::string word;
    std::string prev_word;
    bool first_s = true;
    size_t size = 0;
    size_t block_size = 0;

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
            block_size = word.length() + 1; //since a new block starts
        }
        else{
            //compress the string with rear coding and, if adding this size to the size of the current block,
            //the size of the block become bigger than logical_block_size does not insert it in the current block but 
            //as the first uncompressed string of the next block

            //create the compressed substring associated with the new string
            auto lcp = compute_lcp(prev_word, word);
            auto rear_length = prev_word.length() - lcp;

            std::string compressed_word = "";
            encode_int(rear_length, compressed_word);
            compressed_word.append(word, lcp);

            //+1 for the '\0' at the end of the new compressed string
            if(block_size + compressed_word.length() + 1 > logical_block_size){
                //create a new full block adding its size to the total size of the storage 
                size += logical_block_size;
                //the size of the strings inserted in the current block is the uncompressed length of the string + the \0
                block_size = word.length() + 1;
            }
            else{ 
                //append the compressed string to the previous one in the block
                block_size += compressed_word.length() + 1; //+1 for the '\0'
            }
        }
        prev_word = word;
    }

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
    output_file += "_B_"+ std::to_string(logical_block_size) +".txt";
}





//create the storage composed of blocks of string with fized length compressed with rear coding and write this storage 
//on file referred by fd
// - input: file from which read the (uncompressed) dictionary of strings; the logical block size; the file descriptor of the 
//file where to write the storage; the vector where write blocks rank that for each block store the number of strings 
//that preceed it 
void write_compressed_strings(char* filename, int logical_block_size, int fd, std::vector<size_t>& blocks_rank){ //, std::vector<size_t>& off_logical_block

    std::string word;
    std::string prev_word;
    size_t n_strings = 0;
    std::string str_logical_block;
    size_t busy_bytes = 0;

    //string composed by just one character that is the ending flag: 0 encoded as the rear coding ((10000000)_2)
    //used to separate the strings encoded in the blocks from the padded bits (0) at its end
    std::string ending_flag; 
    encode_int(0, ending_flag);

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
            str_logical_block = word;
            str_logical_block.push_back('\0');
            //before the first block there are no strings (0)
            blocks_rank.push_back(n_strings);
        }
        else{

            //compress the string with rear coding
            auto lcp = compute_lcp(prev_word, word);
            auto rear_length = prev_word.length() - lcp;

            std::string compressed_word = "";
            encode_int(rear_length, compressed_word);
            compressed_word.append(word, lcp);

            //check if adding the compressed string to the current block the logical block size is exceeded
            if(str_logical_block.length() + compressed_word.length() + 1 > logical_block_size){ 
                //create a new block that starts with the uncompressed string

                if(str_logical_block.length()>logical_block_size){
                    str_logical_block = str_logical_block.substr(0,logical_block_size);
                    str_logical_block[logical_block_size-1] = '\0';
                    std::cout<<"WARNING: the first string of a block is truncated (longer than the block size)"<<std::endl;
                }

                char * writing_data = str_logical_block.data();
                size_t writing_length = str_logical_block.length();
                //indicate the offset of the file where starts the writing and correspond at the end of the last written block
                size_t writing_offset = busy_bytes;
                
                //write the current block (possibly composed by more strings) on file
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

                //if the block is not completely full add and ending byte as flag to signal that there are no more strings. 
                if(str_logical_block.length()< logical_block_size){
                    //https://linux.die.net/man/2/pread
                    wrote = pwrite(fd, ending_flag.data(), 1, writing_offset);
                    if(wrote == -1){
                        std::cout<<"ERROR IN THE WRITING OF THE ENDING FLAG IN THE FILE"<<std::endl;
                        std::cout << "writing failure: " << std::strerror(errno) << '\n';
                        exit(0);
                    }
                }

                busy_bytes += logical_block_size;

                str_logical_block = word;
                str_logical_block.push_back('\0');

                blocks_rank.push_back(n_strings);
            }
            else{ 
                str_logical_block.append(compressed_word);
                str_logical_block.push_back('\0');
            }
        }

        prev_word = word;
        n_strings++;
    }


    //manage the last block
    char * writing_data = str_logical_block.data();
    size_t writing_length = str_logical_block.length();
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


    if(str_logical_block.length()< logical_block_size){
        wrote = pwrite(fd, ending_flag.data(), 1, writing_offset);
        if(wrote == -1){
            std::cout<<"ERROR IN THE WRITING OF THE ENDING FLAG IN THE FILE"<<std::endl;
            std::cout << "writing failure: " << std::strerror(errno) << '\n';
            exit(0);
        }
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

}




//create the storage with fixed blocks having the input file and the (fixed) size of blocks
// - input: the name of the file with the strings; the logical block size; the vector where to write the block ranks
//(i.e. how many strings preceed each block)
void create_storage(char * filename, int logical_block_size, std::vector<size_t>& blocks_rank){//, std::vector<size_t>& off_logical_block

    // 1 : compute the space needed to allocate to write the compressed strings
    size_t compressedSpace = compute_disk_space(filename, logical_block_size);
    std::cout << "disk space "<< compressedSpace << std::endl;

    // 2: find the name of the file where write
    std::string file_aligned;
    name_writing_file(filename, file_aligned, logical_block_size);

    // 3: open the file and allocate the needed space
    int fd = open_file(file_aligned);
    allocate_file(fd, 0, compressedSpace); 

    // 4: write the strings in the file
    write_compressed_strings(filename, logical_block_size, fd, blocks_rank);
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
    output_file += "_B_"+ std::to_string(logical_block_size) +"_blocksRank.txt";
}

//automatically generate the name of file where to are stored the block ranks
// - input: the name of file with the blockwise storage; the string where to write the name
void name_reading_blocks_rank(char* input_file, std::string& output_file){
    std::string input(input_file);
    int pos_end = input.find_last_of('.');
    output_file = input.substr(0, pos_end); 
    output_file += "_blocksRank.txt";
}


//write on disk the block ranks on file
// - input: the name of file with the strings; the vector of block ranks; the logical block size
void write_file_block_ranks(char* input_file, std::vector<size_t>& blocks_rank, int logical_block_size){
    std::string output_name;
    name_writing_blocks_rank(input_file, output_name, logical_block_size);
    vector_to_file(blocks_rank, output_name);
}
