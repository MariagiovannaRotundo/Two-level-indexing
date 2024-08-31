#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <stdio.h>
#include <algorithm>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/vectors.hpp>
#include <math.h>
#include <sux/bits/SimpleSelectZeroHalf.hpp>

#include "mmap_level_fixed_blocks.hpp"



struct bfs_node{
    size_t i;
    size_t j;
    size_t depth;
    size_t depth_parent; //depth of the parent
    size_t pos_parent;
};


//needed for caching
struct freq_infos{
    size_t pos_louds;
    size_t freq;
    size_t len;
    std::string substring;
    size_t rank_0;
};



//0: i >=j; 1: i<j //sort edges by decreasing frequencies
bool operator<(const freq_infos& i, const freq_infos& j) { 

    //put the edges with len = 1 at the end (the frequency does not matter)
    if(i.len == 1)
        return 0;

    if(j.len == 1)
        return 1;

    if(i.freq > j.freq) 
        return 1;
    
    if(i.freq < j.freq)
        return 0; //i < j: i after j
    
    if(i.freq == j.freq){
        if(i.pos_louds > j.pos_louds)
            return 0; //i bigger than j. i<j
        else
            return 1;
    }
    
    return 0; //not reached
}


class LOUDS_PatriciaTrie {
    public:   

    sdsl::bit_vector louds;
    std::vector<unsigned char> labels;
    sdsl::int_vector<> lengths;
    sdsl::int_vector<> values;
    sdsl::rank_support_v<10, 2> rank_10; //used to count internal nodes using louds sequence

    //method to create the Patricia trie
    // - input: a vector with the strings to use for the building; a vector of pointers to elaf in the order the leaves
    void build_trie(std::vector<std::string>& wordList, std::vector<freq_infos>& sub_str){
        
        /* 4 is a good number beacuse the number of 0s is equal to the number of strings + the 
        number of internal nodes. For each internal node there is an entering arc except for the root.
        There is an entering arc also for the nodes that represent the strings. So, the number of
        used bit is equal to the number of (strings + the number of internal nodes that corresponds to 
        the 0s) * 2 - 1 (that corresponds to the 1s and the root has not an entering arc).
        The number of internal nodes is O(n) so, we have (n+O(n))*2-1 = O(4n). 
        So, we use 4*n as upper bound.
        */
        sdsl::bit_vector louds_tmp = sdsl::bit_vector(4 * wordList.size(), 0);
        sdsl::int_vector<> lengths_tmp(wordList.size());
        sdsl::int_vector<> values_tmp(wordList.size());

        bfs_node b;
        b.i = 0;
        b.j = (int)wordList.size();
        b.depth = 0;
        b.depth_parent = 0;
        b.pos_parent = 0;

        std::queue<bfs_node> bfs_visit; //queue used to visit "nodes" of the trie in LOUDS order
        bfs_visit.push(b);

        int p = 0; //position in louds_tmp bit vector
        int v = 0;
        int l = 0;

        while(!bfs_visit.empty()){

            std::queue<int> indexes;
            std::queue<int> level_labels;

            bfs_node b = bfs_visit.front();

            if(b.i>=b.j){ //there is an error
                std::cout<<"Error "<<std::endl;
                break;
            }
            if(b.j-b.i==1){ //the node has not children but a string (value), a leaf
                values_tmp[v++] = b.i;
                p++; //by default all bits in louds_tmp[p] there is 0. So, only increment p
                bfs_visit.pop();  
                continue;
            }

            //get the letter in position d (d^th char) in the first word
            unsigned char last = (unsigned char) wordList[b.i][b.depth]; 

            //localize all the subtries starting in the node we are examinating
            indexes.push(b.i);
            level_labels.push(last);
            for(int k=b.i+1; k<b.j; k++){
                if((unsigned char) wordList[k][b.depth] != last){
                    last = (unsigned char)wordList[k][b.depth];
                    indexes.push(k);
                    level_labels.push(last);
                }
            }
            indexes.push(b.j);

            //if for all strings the d-th caracter is the same
            if(level_labels.size() == 1){
                if(b.depth==0){//it is the root node
                    louds_tmp[p] = 1;
                    sub_str[p].substring.push_back(level_labels.front());

                    p+=2;

                    lengths_tmp[l++] = 0;
                    labels.push_back(level_labels.front());

                    bfs_visit.front().depth_parent = 0;
                    bfs_visit.front().depth += 1;
                    
                }
                else{//count the same node later because the tree is compressed
                    bfs_visit.front().depth += 1;

                    sub_str[bfs_visit.front().pos_parent].substring.push_back(level_labels.front());

                } 
            }
            else{//look at all the subtries 
                bfs_visit.pop();   
                lengths_tmp[l++] = b.depth - b.depth_parent;

                while(!level_labels.empty()){

                    //insert in LOUDS a 1 for 
                    louds_tmp[p++] = 1;
                    sub_str[p-1].substring.push_back(level_labels.front());

                    labels.push_back(level_labels.front());
                    level_labels.pop();

                    //for each subtrie
                    bfs_node bc;
                    bc.i = indexes.front();
                    indexes.pop();
                    bc.j = indexes.front();
                    bc.depth_parent = b.depth;
                    bc.depth = b.depth+1;
                    bc.pos_parent = p-1;
                    bfs_visit.push(bc);
                }
                p++;//set the 0 as last bit for that node
            }
        }

        values_tmp.resize(v);
        lengths_tmp.resize(l);
        louds_tmp.resize(p);

        labels.shrink_to_fit();

        sdsl::util::bit_compress(values_tmp);  
        sdsl::util::bit_compress(lengths_tmp);  

        louds = louds_tmp;
        lengths = lengths_tmp;
        values = values_tmp;

        sdsl::rank_support_v<10, 2> rank_10_temp(&louds);

        rank_10 = rank_10_temp;


        sub_str.resize(p);

    }


    sdsl::bit_vector getLouds(){
        return louds;
    }

    size_t getLabelsSize(){
        return labels.size();
    }


    //used in the recursive ocmputation of the height of the trie
    // TO NOT USE DIRECTLY. CALLED BY get_heigth()
    size_t height(std::vector<std::string>& wordList, size_t i, size_t j, size_t d){

        //error with the parameters
        if(i>=j)
            return -1;
        //only one string in the range 
        if(j-i==1) 
            return 0;
        
        size_t h = 0;
        std::queue<size_t> indexes;
        unsigned char last = (unsigned char) wordList[i][d];

        //localize the subtries
        indexes.push(i);
        for(size_t k=i+1; k<j; k++){
            if((unsigned char)wordList[k][d] != last){
                last = (unsigned char) wordList[k][d];
                indexes.push(k);
            }
        }
        indexes.push(j);

        //if for all strings the d-th caracter is the same
        if(indexes.size() == 2){
            if(d==0){//consider the root node in the height
                h = 1 + height(wordList, i, j, d+1);
            }
            else{//count the same node later because the tree is compressed
                h = height(wordList, i, j, d+1);
            }
        }
        else{//look at all the subtries 
            while(indexes.size()>1){
                size_t first = indexes.front();
                indexes.pop();
                size_t hc = 1 + height(wordList, first, indexes.front(), d+1);
                if(hc > h) h = hc;
            }
        }

        return  h;
    }


    //to get height of the patricia trie
    // - input: the set of strings used for the building of the Patricia Trie
    size_t get_heigth(std::vector<std::string>& wordList) {
        
        size_t h = height(wordList, 0, wordList.size(), 0);

        return h;
    }




    //to get the size in bytes of the patricia trie and the data structure to traverse it and needed for rank operations
    // - input: the select_0 data structure; the vector of block ranks 
    size_t getSize(sux::bits::SimpleSelectZeroHalf<>& select_0, sdsl::int_vector<>& blocks_rank){

        size_t size = sdsl::size_in_bytes(louds);
        size += labels.capacity() * sizeof(char);
        size += sdsl::size_in_bytes(lengths);
        size += sdsl::size_in_bytes(values);
        size += select_0.bitCount() / 8; // in bytes, not bits
        size += sdsl::size_in_bytes(rank_10);
        size += sdsl::size_in_bytes(blocks_rank);

        return size;
    }


    //to get the 0 that follows the indicated position in LOUDS
    // - input: the position from which to start a 0
    inline int get_end_zero(int start){
        //at start louds[start]=1
        while(louds[start] == 1){ 
            start++;
        }
        return start;
    }


    //search the character s in the labels that starts from index in the vector of label. start is the position in 
    // LOUDS of the label in labels[index].
    // - input: the index of the first label of the node in labels sequence; the position in LODUS;
    // the character to search
    inline int low_linear_search(int index, int start, unsigned char s){
        int pos = index; //position of the just smaller char respect to s char
        int i = index + 1; //pick the first label as minimum and start the analysis from the second label
        while(louds[++start] == 1){//++start because start from the 1 corresponding to the 2nd arc (2nd label)
            if (labels[i] < s){
                pos = i;
            }
            else if (labels[i] > s){
                return pos;
            }
            else{
                return i;
            }
            i++;
        }
        return pos;
    }




    //to find the just left block of the current one
    //DO NOT USE DIRECTLY. FOR LOOKUP FUNCTION
    int get_left_block(int * start_g, int x, sux::bits::SimpleSelectZeroHalf<>& select_0, 
        std::vector<size_t>& seen_arcs, size_t seen){

        int rank, index;
        int start = *start_g;

        rank = start - x + 1;
        index = x -1;

        //start here is the position of the 1 that represent the choosen arc in LOUDS. 
        while(start-1 >= 0){ 
            
            if(louds[start-1] == 1){ //start is the position of the node in the parent encoding

                start = select_0.selectZero(index-1)+1; //get the position where the "left node" starts
                rank = index;  

                //get the most right block in the subtrie
                while(louds[start] == 1){
                    //take always the last arc of the node
                    index = get_end_zero(start) - rank;
                    start = select_0.selectZero(index-1)+1;
                    rank = index;  
                }
                *start_g = start;
                return index; 
            }
            else{//go to the parent and look for the left subtrie in the next iteration

                //check if we already are in the root so the parent does not exist
                if (rank == 0){ //in the root: no parent exist: not left block exists
                    start = 0;
                    break;
                }
                else{
                    start = seen_arcs[--seen]; //position of current node in the parent node
                    index = rank - 1 ;  
                    rank = start - rank + 1; 
                }
            }   
        }
        return -1;
    }



    //look for a string s in the storage level returning its rank
    // - input: content of the mmap; string to search; select_0; stack that will be used for the upward traverse of the trie;
    // vector with the ranks of the blocks; size of the logical blocks
    size_t lookup(MmappedSpace& mmap_space, const std::string &s, sux::bits::SimpleSelectZeroHalf<>& select_0, 
        std::vector<size_t>& seen_arcs, sdsl::int_vector<>& blocks_rank, int logical_block_size){

        int start = 0; //position in which a node starts in louds encoding
        size_t d = 0; //used for depth in the trie
        size_t nleaves, n, index, rank, i; 
        char * s_block;
        seen_arcs.clear();
        size_t seen = 0;

        if(louds.size() == 1){//the tree has ony one block
            int equal = std::strcmp(s.data(), mmap_space.access_at(0));
            if(equal == -1) //s is smaller of the 1st string in the block
                return -1; 
            else if(equal == 0)
                return 1;
            else
                return mmap_space.scan_block(s, 0, blocks_rank[1]- blocks_rank[0], 0);
        }

        //here louds[start] is equal to 1
        rank = 0;
        index = 0;

        //while is not a leaf
        while(louds[start] == 1){

            d += lengths[rank_10(start)]; 
            
            if(s.length() < d){
                //take the left-most leaf in the current subtree
                seen_arcs[seen++] = start;
                start = select_0.selectZero(index)+1;
                rank = index+1;
                index = start - rank;  
            }
            else{   
                i = low_linear_search(index, start, (unsigned char) s[d]); 

                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;
            }
            
        }
        //here louds[start] == 0 

        //the current node is a leaf (a block is identified)  
        nleaves = rank - rank_10(start);
        //access to the first string of the block
        s_block = mmap_space.access_at(values[nleaves] * logical_block_size);
        
        //compute where is the mismatch between the string of the block and the searched one
        int lcp = 0;
        while (s[lcp] != '\0' && s_block[lcp] != '\0' && s[lcp] == s_block[lcp])
            lcp++;

        auto len = strlen(s_block);
        
        if(lcp == s.length() && s.length() == len){ //string found
            return blocks_rank[values[nleaves]] +1;
        }

        bool compare = (unsigned char)s[lcp] < (unsigned char)s_block[lcp]; 
        
        if(lcp > d && !compare){ //scan the current block
            auto off = values[nleaves]; 
            return mmap_space.scan_block_no_first(s, lcp, len, off* logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]);
        }
        //upward traversal

        int x;
        
        while(true){

            x = rank;
            //compute the position of the current node in the parent node encoding
            start = seen_arcs[--seen]; 
            rank = start - x + 1;    

            if(lcp < d){ //d (depth) is always > 0
                d -= lengths[rank_10(start)]; //compute the length at the parent node
            }
            else {
                if(compare){

                    index = get_left_block(&start, x, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - rank_10(start); // count also the current node that is a leaf
                
                    auto off = values[nleaves];
                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 
                }
                else{
                    index = x - 1;  
                    i = low_linear_search(index, start, (unsigned char) s[d]); 

                    start = select_0.selectZero(i)+1;
                    rank = i+1; 

                    //take the block to the extremely right in the current subtrie
                    while(louds[start] == 1){
                        //take always the last arc of the node
                        index = get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 
                    }

                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - rank_10(start); 

                    auto off = values[nleaves-1];
                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1]-blocks_rank[off], blocks_rank[off]); 
                }
            }
        } 
    }



    //to compute the number of page faults and of random I/O
    size_t lookup_mincore(MmappedSpace& mmap_space, const std::string &s, sux::bits::SimpleSelectZeroHalf<>& select_0, 
        std::vector<size_t>& seen_arcs, sdsl::int_vector<>& blocks_rank, int logical_block_size, size_t * fault_page,
        size_t * random_access_to_pages){

        int start = 0; //position in which a node starts in louds encoding
        size_t d = 0; //used for depth in the trie
        size_t nleaves, n, index, rank, i; 
        char * s_block;
        seen_arcs.clear();
        size_t seen = 0;

        //for the mincore function
        char * mmapped_content = mmap_space.get_mmapped_content();
        auto page_size = sysconf(_SC_PAGE_SIZE);
        unsigned char vec[1];
        int res_mincore;

        auto page_for_block = logical_block_size/page_size;
        
        if(louds.size() == 1){//the tree has ony one block
            int equal = std::strcmp(s.data(), mmap_space.access_at(0));
            if(equal == -1) //s is smaller of the 1st string in the block
                return -1; 
            else if(equal == 0)
                return 1;
            else
                return mmap_space.scan_block(s, 0, blocks_rank[1]- blocks_rank[0], 0);
        }

        //here louds[start] is equal to 1
        rank = 0;
        index = 0;

        //while is not a leaf
        while(louds[start] == 1){

            d += lengths[rank_10(start)]; 
            
            if(s.length() < d){
                //take the left-most leaf in the current subtree
                seen_arcs[seen++] = start;
                start = select_0.selectZero(index)+1;
                rank = index+1;
                index = start - rank;  
            }
            else{   
                i = low_linear_search(index, start, (unsigned char) s[d]); 

                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;
            }
            
        }
        //here louds[start] == 0 

        //the current node is a leaf (a block is identified)  
        nleaves = rank - rank_10(start);
        //access to the first string of the block

        res_mincore = mincore(mmapped_content + values[nleaves] * logical_block_size, page_size, vec);
        if(res_mincore == -1){
            std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
            std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
        }
        if((int)(vec[0] & 1) == 0){
            (*fault_page)++;
        }
        (*random_access_to_pages)++;

        size_t first_off = values[nleaves];

        s_block = mmap_space.access_at(values[nleaves] * logical_block_size);
        
        //compute where is the mismatch between the string of the block and the searched one
        int lcp = 0;
        while (s[lcp] != '\0' && s_block[lcp] != '\0' && s[lcp] == s_block[lcp])
            lcp++;

        auto len = strlen(s_block);
        
        if(lcp == s.length() && s.length() == len){ //string found
            return blocks_rank[values[nleaves]] +1;
        }

        bool compare = (unsigned char)s[lcp] < (unsigned char)s_block[lcp]; 
        
        if(lcp > d && !compare){ //scan the current block
            auto off = values[nleaves]; 

            res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
            if(res_mincore == -1){
                std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
            }
            if((int)(vec[0] & 1) == 0){
                (*fault_page)++;
            }

            return mmap_space.scan_block_no_first(s, lcp, len, off* logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]);
        }
        //upward traversal

        int x;
        
        while(true){

            x = rank;
            //compute the position of the current node in the parent node encoding
            start = seen_arcs[--seen]; 
            rank = start - x + 1;    

            if(lcp < d){ //d (depth) is always > 0
                d -= lengths[rank_10(start)]; //compute the length at the parent node
            }
            else {
                if(compare){

                    index = get_left_block(&start, x, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - rank_10(start); // count also the current node that is a leaf
                
                    auto off = values[nleaves];

                    res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                    if(res_mincore == -1){
                        std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                        std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                    }
                    if((int)(vec[0] & 1) == 0){
                        (*fault_page)++;
                    }
                    if(abs(int(first_off - off)) > 1){
                        (*random_access_to_pages)++;
                    }

                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 
                }
                else{
                    index = x - 1;  
                    i = low_linear_search(index, start, (unsigned char) s[d]); 

                    start = select_0.selectZero(i)+1;
                    rank = i+1; 

                    //take the block to the extremely right in the current subtrie
                    while(louds[start] == 1){
                        //take always the last arc of the node
                        index = get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 
                    }

                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - rank_10(start); 

                    auto off = values[nleaves-1];

                    res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                    if(res_mincore == -1){
                        std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                        std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                    }
                    if((int)(vec[0] & 1) == 0){
                        (*fault_page)++;
                    }
                    if(abs(int(first_off - off)) > 1){
                        (*random_access_to_pages)++;
                    }

                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1]-blocks_rank[off], blocks_rank[off]); 
                }
            }
        } 
    }



    //look for a string s in the storage level returning its rank
    // - input: content of the mmap; string to search; select_0; stack that will be used for the upward traverse of the trie;
    // vector with the ranks of the blocks; size of the logical blocks
    size_t lookup_no_scan(MmappedSpace& mmap_space, const std::string &s, sux::bits::SimpleSelectZeroHalf<>& select_0, 
        std::vector<size_t>& seen_arcs, sdsl::int_vector<>& blocks_rank, int logical_block_size){

        int start = 0; //position in which a node starts in louds encoding
        size_t d = 0; //used for depth in the trie
        size_t nleaves, n, index, rank, i; 
        char * s_block;
        seen_arcs.clear();
        size_t seen = 0;
        
        if(louds.size() == 1){//the tree has ony one block
            int equal = std::strcmp(s.data(), mmap_space.access_at(0));
            if(equal == -1) //s is smaller of the 1st string in the block
                return -1; 
            else if(equal == 0)
                return 1;
            else
                return mmap_space.scan_block(s, 0, blocks_rank[1]- blocks_rank[0], 0);
        }

        //here louds[start] is equal to 1
        rank = 0;
        index = 0;

        //while is not a leaf
        while(louds[start] == 1){

            d += lengths[rank_10(start)]; 
            
            if(s.length() < d){
                //take the left-most leaf in the current subtree
                seen_arcs[seen++] = start;
                start = select_0.selectZero(index)+1;
                rank = index+1;
                index = start - rank;  
            }
            else{   
                // std::cout<<"linear search"<<std::endl;
                i = low_linear_search(index, start, (unsigned char) s[d]); 
                // std::cout<<"return "<<i<<std::endl;
                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;
            }
            
        }
        //here louds[start] == 0 

        //the current node is a leaf (a block is identified)  
        nleaves = rank - rank_10(start);
        //access to the first string of the block
        s_block = mmap_space.access_at(values[nleaves] * logical_block_size);

        //compute where is the mismatch between the string of the block and the searched one
        int lcp = 0;
        while (s[lcp] != '\0' && s_block[lcp] != '\0' && s[lcp] == s_block[lcp])
            lcp++;

        auto len = strlen(s_block);
        
        if(lcp == s.length() && s.length() == len){ //string found
            return blocks_rank[values[nleaves]] +1;
        }

        bool compare = (unsigned char)s[lcp] < (unsigned char)s_block[lcp]; 
        
        if(lcp > d && !compare){ //scan the current block
            auto off = values[nleaves]; 
            return 1;
        }
        //upward traversal

        int x;
        
        while(true){

            x = rank;
            //compute the position of the current node in the parent node encoding
            start = seen_arcs[--seen]; 
            rank = start - x + 1;    

            if(lcp < d){ //d (depth) is always > 0
                d -= lengths[rank_10(start)]; //compute the length at the parent node
            }
            else {
                if(compare){

                    index = get_left_block(&start, x, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - rank_10(start); // count also the current node that is a leaf
                
                    auto off = values[nleaves];

                    return 1; 
                }
                else{
                    index = x - 1;  
                    i = low_linear_search(index, start, (unsigned char) s[d]); 

                    start = select_0.selectZero(i)+1;
                    rank = i+1; 

                    //take the block to the extremely right in the current subtrie
                    while(louds[start] == 1){
                        //take always the last arc of the node
                        index = get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 
                    }

                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - rank_10(start); 

                    auto off = values[nleaves-1];

                    return 1; 
                }
            }
        } 
    }


};
