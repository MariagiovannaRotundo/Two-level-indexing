#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <stdio.h>
#include <algorithm>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/vectors.hpp>
#include <math.h>
#include <limits>
#include <time.h>

#include "patricia_trie.hpp"



struct block_occurrences{
    size_t num_block;
    size_t occ;
};



class Cache {

    public:

    sdsl::bit_vector isInCache;
    sdsl::int_vector<> text_pointer;
    std::string text_edges; 
    sdsl::rank_support_v<1, 1> rank_1_cache;


    size_t get_block_string(const std::string &s, LOUDS_PatriciaTrie & louds_pt,
        sux::bits::SimpleSelectZeroHalf<>& select_0, std::vector<size_t>& seen_arcs, sdsl::int_vector<>& blocks_rank, 
        int logical_block_size){

        int start = 0; //position in which a node starts in louds encoding
        size_t d = 0; //used for depth in the trie
        size_t nleaves, n, index, rank, i; 
        char * s_block;
        seen_arcs.clear();
        size_t seen = 0;

        if(louds_pt.louds.size() == 1){//the tree has ony one block
            return 0;
        }

        //here louds[start] is equal to 1
        rank = 0;
        index = 0;

        //while is not a leaf
        while(louds_pt.louds[start] == 1){

            d += louds_pt.lengths[louds_pt.rank_10(start)]; 

            if(s.length() < d){
                //take the left-most leaf in the current subtree
                seen_arcs[seen++] = start;
                start = select_0.selectZero(index)+1;
                rank = index+1;
                index = start - rank;  
            }
            else{   
                i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;
            }
            
        }
        //here louds[start] == 0 

        //the current node is a leaf (a block is identified)  
        nleaves = rank - louds_pt.rank_10(start);
        //access to the first string of the block

        return louds_pt.values[nleaves];
        
    }

    


    void compute_block_frequencies(std::vector<size_t>& frequencies, LOUDS_PatriciaTrie & louds_pt, 
        char* filename_query, sux::bits::SimpleSelectZeroHalf<>& select_0, sdsl::int_vector<>& blocks_rank, 
        int logical_block_size, size_t heigth, MmappedSpace& mmap_space){

        std::vector<size_t> seen_arcs;
        seen_arcs.resize(heigth);

        std::cout<<"computing access frequency of blocks"<<std::endl;

        std::ifstream ifs(filename_query, std::ifstream::in); 
        if (!ifs){
            std::cout<<"cannot open "<<filename_query<<"\n"<<std::endl;
            exit(0);
        }
    
        // size_t num_random_io;

        std::string word;
        //read all the words by skipping empty strings
        while (getline(ifs, word)){
           
            if (word == "") continue;
           
            seen_arcs.clear();
            //search the string in the trie
            size_t reached_block = get_block_string(word, louds_pt, select_0, seen_arcs, blocks_rank, logical_block_size);
            frequencies[reached_block]++;
            
        }
    }




    void compute_freq_edges(std::vector<std::string>& first_strings, std::vector<size_t>& frequencies,
        LOUDS_PatriciaTrie & louds_pt, sux::bits::SimpleSelectZeroHalf<>& select_0, std::vector<freq_infos>& freq_len){
        //sdsl::int_vector<>& freq_nodes, sdsl::int_vector<>& length_nodes){

        int start; //position in which a node starts in louds encoding
        int d; //depth in the string s (and in the trie)
        int end, nleaves, n, index, rank, i; //end is the position of the 0 associated to the node
        std::vector<int> seen_arcs;
        std::string s;
        auto louds = louds_pt.getLouds();

        std::cout<<"computing edge frequencies ..."<<std::endl;

        for(int j=0; j<first_strings.size(); j++){ //looking for each string in the trie

            s = first_strings[j];

            start = 0, d = 0;
            rank = 0; //rank_0 on louds
            index = 0;

            seen_arcs.clear();
           
            //if the tree has ony one block
            if(louds.size() == 1){
                std::cout<<"ONLY ONE STRING IN THE TRIE: NO CACHING "<<std::endl;
                exit(0);
            }

            //here louds[start] is equal to 1

            int pre_start = -1;

            //while is not a leaf
            while(louds[start] == 1){

                d += louds_pt.lengths[louds_pt.rank_10(start)]; //length associated to the current node for the blind search

                if(pre_start!=-1){
                    freq_len[pre_start].len = louds_pt.lengths[louds_pt.rank_10(start)]; //len of the edge
                }

                if(s.length() < d){
                    //take the left-most leaf in the current subtree
                    seen_arcs.push_back(start);
                    freq_len[start].rank_0 = rank;  

                    pre_start = start;

                    start = select_0.selectZero(index)+1;
                    rank = index+1;
                    index = start - rank;

                }
                else{   
                    i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                    pre_start = start + i - index;

                    //index in louds of the chosen 1 (chosen arc to go down in the trie)
                    seen_arcs.push_back(start + i - index);

                    freq_len[start + i - index].rank_0 = rank;  

                    start = select_0.selectZero(i)+1; //i-th children node //i+1 to count the current 1
                    rank = i+1; //rank0(start)
                    //position where the current node starts in labels vector
                    index = start - rank; //this does not count the current 1 //rank1(start)
                }
            }

            nleaves = rank - louds_pt.rank_10(start);

            std::string s_block = first_strings[louds_pt.values[nleaves]]; //access to the first string of the block
            
            if(first_strings[j].compare(s_block) != 0){
                std::cout<<"errore: found a different string!"<<std::endl;
                exit(0);
            }

            //start from 1 because the first length (associated with root) is always 0
            for(int k = 1; k<seen_arcs.size(); k++){
                freq_len[seen_arcs[k-1]].freq += frequencies[j];
                
            }    
        }

    }

        
    size_t getCacheSize(){

        size_t size = sdsl::size_in_bytes(isInCache);
        size += sdsl::size_in_bytes(text_pointer);
        size += text_edges.length();
        size += sdsl::size_in_bytes(rank_1_cache);

        // std::cout<<"\nsize isInCache "<<sdsl::size_in_bytes(isInCache)<<std::endl;
        // std::cout<<"size text_pointer "<< sdsl::size_in_bytes(text_pointer)<<std::endl;
        // std::cout<<"size text_edges "<<text_edges.length()<<std::endl;
        // std::cout<<"size rank_1_cache "<<sdsl::size_in_bytes(rank_1_cache)<<std::endl;

        return size;
    }



    void select_edges(std::vector<freq_infos>& freq_len, LOUDS_PatriciaTrie & louds_pt, size_t cache_size){

        //sort the edges ordering them by decreasing frequences. Put the edges with length in the ended part of the vector
        sort(freq_len.begin(), freq_len.end());

        std::cout<<"filling the cache ..."<<std::endl;

        //to have an approximation of the space used in cache
        size_t max_pos_in_cache = 0;
        size_t len_text = 0;

        sdsl::bit_vector isInCache_tmp(louds_pt.getLabelsSize()); //one bit for each element in label vector

        auto size_in_cache = sdsl::size_in_bytes(isInCache_tmp);

        if(size_in_cache >= cache_size){
            std::cout<<"Impossible to keep edges in cache: too low available space"<<std::endl;
            exit(0);
        }

        std::vector<std::string> text_tmp_all(louds_pt.getLabelsSize(), "");
        int i = 0; 
        int pointers = 0;
	    size_t rank_space_estimation = size_in_cache/4; ////https://simongog.github.io/assets/data/sdsl-cheatsheet.pdf

        size_in_cache += rank_space_estimation; //consider also the additional space for rank operations

        //freq_len[i].len > 1 exclude the edges with len 1 and 0 (the leaves that have frequency equal to 0).
        while (i<freq_len.size() && freq_len[i].freq > 0 && freq_len[i].len > 1){//the sum compute the bits (no bytes)

            len_text += freq_len[i].substring.length();; 
            isInCache_tmp[freq_len[i].pos_louds - freq_len[i].rank_0] = 1; //set that that edge is in cache


            if((size_in_cache + len_text  + i*(log2(len_text)+1)/8) > cache_size){ //label size for the rank
                //stop putting edges in cache
                isInCache_tmp[freq_len[i].pos_louds - freq_len[i].rank_0] = 0; //set that that edge is not in cache

                if(i==0){ //we put no edges in cache because it's already full
                    std::cout<<"Impossible to keep edges in cache: too low available space"<<std::endl;
                    exit(0);
                }
                break;
            }
            else{
                text_tmp_all[freq_len[i].pos_louds - freq_len[i].rank_0].append(freq_len[i].substring, 1);
                i++;
            }
        }

    
        size_t t = 0; // to move in the text_pointer vector

        sdsl::int_vector<> text_pointer_tmp(louds_pt.getLabelsSize());
        text_edges = ""; 

        for(int j = 0; j < louds_pt.getLabelsSize(); j++){

            if(text_tmp_all[j].compare("") != 0){

                text_pointer_tmp[t++] = text_edges.size();
                text_edges.append(text_tmp_all[j]); //skip the first char that is aready stored in the label vector
                text_edges.push_back('\0');

            } 
        }

        text_tmp_all.clear();
        text_tmp_all.shrink_to_fit();
        text_pointer_tmp.resize(t);

        isInCache = isInCache_tmp;
        text_pointer = text_pointer_tmp;

        sdsl::util::bit_compress(text_pointer);

        sdsl::rank_support_v<1, 1>  rank_1_cache_tmp(&isInCache);
        rank_1_cache = rank_1_cache_tmp;
    }



    size_t lookup(MmappedSpace& mmap_space, const std::string &s, sux::bits::SimpleSelectZeroHalf<>& select_0, 
        std::vector<size_t>& seen_arcs, sdsl::int_vector<>& blocks_rank, int logical_block_size, LOUDS_PatriciaTrie & louds_pt){

        int start = 0; //position in which a node starts in louds encoding
        size_t d = 0; //used for depth in the trie
        size_t nleaves, n, index, rank, i; 
        char * s_block;
        seen_arcs.clear();
        size_t seen = 0;

        size_t s_len = s.length();

        bool path_in_cache = true;
        size_t lcp_cache = 0;

        size_t child_len;
        
        if(louds_pt.louds.size() == 1){//the tree has ony one block
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
        while(path_in_cache && louds_pt.louds[start] == 1){

            d += louds_pt.lengths[louds_pt.rank_10(start)]; 
            
            if(s_len < d){

                start = seen_arcs[--seen]; 

                index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                if(index == -1)
                    return -1;
    
                rank = index;
                nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
            
                auto off = louds_pt.values[nleaves];
                return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 

            }
            else{   

                i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;

                int first_char = (unsigned char)s[d] - louds_pt.labels[i];

                if(first_char > 0){//((unsigned char)s[d] > louds_pt.labels[i]){ //seleziona come blocco quello più a destra nel subtrie

                    while(louds_pt.louds[start] == 1){
                        //take always the last edge of the node
                        index = louds_pt.get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 
                    }
                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - louds_pt.rank_10(start); 

                    auto off = louds_pt.values[nleaves-1];
                    return mmap_space.scan_block_with_lcp(s, d, off * logical_block_size, blocks_rank[off+1]-blocks_rank[off], blocks_rank[off]); 
                }
                
                if(first_char < 0){ // take the block to the left of the first block of the subtrie
                    start = seen_arcs[--seen]; 

                    index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                
                    auto off = louds_pt.values[nleaves];
                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 

                }

                //s[d] == labels[i] and long edges are in cache
                lcp_cache += first_char; //add 1 because s[d] == labels[i] 

                if(louds_pt.louds[start] == 0){
                    break;
                }
                
                //for sure louds_pt.louds[start] == 1
                child_len = louds_pt.lengths[louds_pt.rank_10(start)];

                if(child_len > 1){

                    if(isInCache[i] == 0){ //if the edge is not in the cache continue without considering cache anymore
                        path_in_cache = false;
                        break;
                    }

                    //else: is in cache
                    char * edge_s = &text_edges[text_pointer[rank_1_cache(i)]]; //does not count the current 1
                    int j = 0;

                    int compare;
                
                    while(path_in_cache && d+j+1 < s_len && (edge_s + j)[0]!='\0'){
                        compare = (unsigned char)s[d+j+1] - (unsigned char)((edge_s + j)[0]);
                        path_in_cache = ( compare == 0 ); //1 (true) if they are equal; 0(false) otherwise
                        j += path_in_cache; //if they are equal path_in_cache=1 so increase j, otherwise j does not change
                    }
                    lcp_cache += j;

                    //if there is a mismatch between s and the label of the edge
                    if(compare > 0){//((unsigned char)s[d+j+1] > (unsigned char)(edge_s + j)[0]){ /take the most right block in the subtrie

                        while(louds_pt.louds[start] == 1){
                            //take always the last edge of the node
                            index = louds_pt.get_end_zero(start) - rank;
                            start = select_0.selectZero(index - 1)+1;
                            rank = index; 
                        }
                        //here louds[start] == 0
                        rank++; 
                        nleaves = rank - louds_pt.rank_10(start); 

                        auto off = louds_pt.values[nleaves-1];
                        return mmap_space.scan_block_with_lcp(s, lcp_cache, off * logical_block_size, blocks_rank[off+1]-blocks_rank[off], blocks_rank[off]); 
                    }
                    if(compare < 0){ // thake the block to the left of the first block of the subtrie
                        start = seen_arcs[--seen]; 

                        index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                        if(index == -1)
                            return -1;
            
                        rank = index;
                        nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                    
                        auto off = louds_pt.values[nleaves];
                        return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 

                    }
                }
            }
        }

        //if you execute the body of the while path_in_cache is false FOR SURE
        while(louds_pt.louds[start] == 1){

            d += louds_pt.lengths[louds_pt.rank_10(start)]; 
            
            if(s_len < d){
                //take the left-most leaf in the current subtree
                seen_arcs[seen++] = start;
                start = select_0.selectZero(index)+1;
                rank = index+1;
                index = start - rank;  //rank1
            }
            else{   
                i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 
                seen_arcs[seen++] = start + i - index;
                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;
            }
        }
        //here louds[start] == 0 
    
        //the current node is a leaf (a block is identified)  
        nleaves = rank - louds_pt.rank_10(start);

        if(path_in_cache && s_len == d && louds_pt.labels[i] =='\0'){
            return blocks_rank[louds_pt.values[nleaves]];
        }

        //access to the first string of the block
        s_block = mmap_space.access_at(louds_pt.values[nleaves] * logical_block_size);

        //compute where is the mismatch between the string of the block and the searched one
        int lcp = lcp_cache; //the first lcp_cache are equal
        while (s[lcp] != '\0' && s_block[lcp] != '\0' && s[lcp] == s_block[lcp])
            lcp++;

        auto len = strlen(s_block);
        
        if(lcp == s_len && s_len == len){ //string found
            return blocks_rank[louds_pt.values[nleaves]] +1;
        }

        bool compare = (unsigned char)s[lcp] < (unsigned char)s_block[lcp]; 

        if(lcp > d && !compare){ //scan the current block (s > s_block)
            auto off = louds_pt.values[nleaves]; 
            return mmap_space.scan_block_no_first(s, lcp, len, off* logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]);
        }
        //upward traversal

        if(path_in_cache){ //scan the previous block (s < s_block)
            auto off = louds_pt.values[nleaves] -1; 
            return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 
        }

        int x;
        
        while(true){

            x = rank;
            //compute the position of the current node in the parent node encoding
            start = seen_arcs[--seen]; 
            rank = start - x + 1;    

            if(lcp < d){ //d (depth) is always > 0
                d -= louds_pt.lengths[louds_pt.rank_10(start)]; //compute the length at the parent node
            }
            else {
                if(compare){

                    index = louds_pt.get_left_block(&start, x, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                
                    auto off = louds_pt.values[nleaves];
                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 
                }
                else{
                    index = x - 1;  
                    i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                    start = select_0.selectZero(i)+1;
                    rank = i+1; 

                    //take the block to the extremely right in the current subtrie
                    while(louds_pt.louds[start] == 1){
                        //take always the last arc of the node
                        index = louds_pt.get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 
                    }

                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - louds_pt.rank_10(start); 

                    auto off = louds_pt.values[nleaves-1];
                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1]-blocks_rank[off], blocks_rank[off]); 
                }
            }
        } 
    }

    


    size_t lookup_no_scan(MmappedSpace& mmap_space, const std::string &s, sux::bits::SimpleSelectZeroHalf<>& select_0, 
        std::vector<size_t>& seen_arcs, sdsl::int_vector<>& blocks_rank, int logical_block_size, LOUDS_PatriciaTrie & louds_pt){

        int start = 0; //position in which a node starts in louds encoding
        size_t d = 0; //used for depth in the trie
        size_t nleaves, n, index, rank, i; 
        char * s_block;
        seen_arcs.clear();
        size_t seen = 0;

        size_t s_len = s.length();

        bool path_in_cache = true;
        size_t lcp_cache = 0;

        size_t child_len;
        
        if(louds_pt.louds.size() == 1){//the tree has ony one block
            int equal = std::strcmp(s.data(), mmap_space.access_at(0));
            if(equal == -1) //s is smaller of the 1st string in the block
                return -1; 
            else if(equal == 0)
                return 1;
            else
                return 1;
        }

        //here louds[start] is equal to 1
        rank = 0;
        index = 0;

        //while is not a leaf
        while(path_in_cache && louds_pt.louds[start] == 1){

            d += louds_pt.lengths[louds_pt.rank_10(start)]; 
            
            if(s_len < d){

                start = seen_arcs[--seen]; 

                index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                if(index == -1)
                    return -1;
    
                rank = index;
                nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
            
                auto off = louds_pt.values[nleaves];
                return 1; 

            }
            else{   

                i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;

                // bool first_char = ((unsigned char)s[d] == louds_pt.labels[i]);
                int first_char = (unsigned char)s[d] - louds_pt.labels[i];

                if(first_char > 0){//((unsigned char)s[d] > louds_pt.labels[i]){ //seleziona come blocco quello più a destra nel subtrie

                    while(louds_pt.louds[start] == 1){
                        //take always the last arc of the node
                        index = louds_pt.get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 
                    }
                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - louds_pt.rank_10(start); 

                    auto off = louds_pt.values[nleaves-1];
                    return 1; 
                }
                
                if(first_char < 0){ 
                    start = seen_arcs[--seen]; 

                    index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                
                    auto off = louds_pt.values[nleaves];
                    return 1; 

                }
                
                //s[d] == labels[i] and long edges are in cache
                lcp_cache += first_char; //add 1 because s[d] == labels[i] 


                if(louds_pt.louds[start] == 0){
                    break;
                }
                
                //for sure louds_pt.louds[start] == 1
                child_len = louds_pt.lengths[louds_pt.rank_10(start)];

                if(child_len > 1){

                    if(isInCache[i] == 0){ 
                        path_in_cache = false;
                        break;
                    }

                    //else: is in cache
                    char * edge_s = &text_edges[text_pointer[rank_1_cache(i)]]; //does not count the current 1
                    int j = 0;

                    int compare;
                
                    while(path_in_cache && d+j+1 < s_len && (edge_s + j)[0]!='\0'){
                        compare = (unsigned char)s[d+j+1] - (unsigned char)((edge_s + j)[0]);
                        path_in_cache = ( compare == 0 ); //1 (true) if they are equal; 0(false) otherwise
                        j += path_in_cache; //if they are equal path_in_cache=1 so increase j, otherwise j does not change
                    }
                    lcp_cache += j;

                    if(compare > 0){//((unsigned char)s[d+j+1] > (unsigned char)(edge_s + j)[0]){

                        while(louds_pt.louds[start] == 1){
                            //take always the last arc of the node
                            index = louds_pt.get_end_zero(start) - rank;
                            start = select_0.selectZero(index - 1)+1;
                            rank = index; 
                        }
                        //here louds[start] == 0
                        rank++; 
                        nleaves = rank - louds_pt.rank_10(start); 

                        auto off = louds_pt.values[nleaves-1];
                        return 1; 
                    }
                    if(compare < 0){ // prendi il blocco a sinistra del primo blocco del subtrie
                        start = seen_arcs[--seen]; 

                        index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                        if(index == -1)
                            return -1;
            
                        rank = index;
                        nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                    
                        auto off = louds_pt.values[nleaves];
                        return 1; 

                    }
                }
            }
        }

        //if you execute the body of the while path_in_cache is false FOR SURE
        while(louds_pt.louds[start] == 1){

            d += louds_pt.lengths[louds_pt.rank_10(start)]; 
            
            if(s_len < d){
                //take the left-most leaf in the current subtree
                seen_arcs[seen++] = start;
                start = select_0.selectZero(index)+1;
                rank = index+1;
                index = start - rank;  //rank1
            }
            else{   
                i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 
                seen_arcs[seen++] = start + i - index;
                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;
            }
        }
        //here louds[start] == 0 
    
        //the current node is a leaf (a block is identified)  
        nleaves = rank - louds_pt.rank_10(start);

        if(path_in_cache && s_len == d && louds_pt.labels[i] =='\0'){
            return blocks_rank[louds_pt.values[nleaves]];
        }

        //access to the first string of the block
        s_block = mmap_space.access_at(louds_pt.values[nleaves] * logical_block_size);

        //compute where is the mismatch between the string of the block and the searched one
        int lcp = lcp_cache; //the first lcp_cache are equal
        while (s[lcp] != '\0' && s_block[lcp] != '\0' && s[lcp] == s_block[lcp])
            lcp++;

        auto len = strlen(s_block);
        
        if(lcp == s_len && s_len == len){ //string found
            return blocks_rank[louds_pt.values[nleaves]] +1;
        }

        bool compare = (unsigned char)s[lcp] < (unsigned char)s_block[lcp]; 

        if(lcp > d && !compare){ //scan the current block (s > s_block)
            auto off = louds_pt.values[nleaves]; 
            return 1;
        }
        //upward traversal

        if(path_in_cache){ //scan the previous block (s < s_block)
            auto off = louds_pt.values[nleaves] -1; 
            return 1; 
        }

        int x;
        
        while(true){

            x = rank;
            //compute the position of the current node in the parent node encoding
            start = seen_arcs[--seen]; 
            rank = start - x + 1;    

            if(lcp < d){ //d (depth) is always > 0
                d -= louds_pt.lengths[louds_pt.rank_10(start)]; //compute the length at the parent node
            }
            else {
                if(compare){

                    index = louds_pt.get_left_block(&start, x, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                
                    auto off = louds_pt.values[nleaves];
                    return 1; 
                }
                else{
                    index = x - 1;  
                    i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                    start = select_0.selectZero(i)+1;
                    rank = i+1; 

                    //take the block to the extremely right in the current subtrie
                    while(louds_pt.louds[start] == 1){
                        //take always the last arc of the node
                        index = louds_pt.get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 
                    }

                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - louds_pt.rank_10(start); 

                    auto off = louds_pt.values[nleaves-1];
                    return 1; 
                }
            }
        } 
    }




    size_t lookup_mincore(MmappedSpace& mmap_space, const std::string &s, sux::bits::SimpleSelectZeroHalf<>& select_0, 
        std::vector<size_t>& seen_arcs, sdsl::int_vector<>& blocks_rank, int logical_block_size, LOUDS_PatriciaTrie & louds_pt,
        size_t * fault_page, size_t * access_to_pages, std::vector<size_t>& accessed_pages_indexes,
        size_t * tot_visited_nodes, std::vector<size_t>& num_nodes, size_t * random_access_to_pages){

        int start = 0; //position in which a node starts in louds encoding
        size_t d = 0; //used for depth in the trie
        size_t nleaves, n, index, rank, i; 
        char * s_block;
        seen_arcs.clear();
        size_t seen = 0;

        size_t s_len = s.length();

        bool path_in_cache = true;
        size_t lcp_cache = 0;

        size_t child_len;

        (*tot_visited_nodes)++; //to count the root
        num_nodes.push_back(start);
        
        // for the mincore function
        char * mmapped_content = mmap_space.get_mmapped_content();
        auto page_size = sysconf(_SC_PAGE_SIZE);
        unsigned char vec[1];
        int res_mincore;
        auto page_for_block = logical_block_size/page_size;

        if(louds_pt.louds.size() == 1){//the tree has ony one block
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
        while(path_in_cache && louds_pt.louds[start] == 1){

            d += louds_pt.lengths[louds_pt.rank_10(start)]; 
            
            if(s_len < d){

                start = seen_arcs[--seen]; 

                (*tot_visited_nodes)++; 
                num_nodes.push_back(start);

                index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                if(index == -1)
                    return -1;
    
                rank = index;
                nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
            
                auto off = louds_pt.values[nleaves];

                accessed_pages_indexes.push_back(off);

                res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                if(res_mincore == -1){
                    std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                    std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                }
                if((int)(vec[0] & 1) == 0){
                    (*fault_page)++;
                }
                (*access_to_pages)++;
                (*random_access_to_pages)++;

                return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 

            }
            else{   

                i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;

                (*tot_visited_nodes)++; 
                num_nodes.push_back(start);

                // bool first_char = ((unsigned char)s[d] == louds_pt.labels[i]);
                int first_char = (unsigned char)s[d] - louds_pt.labels[i];

                if(first_char > 0){//((unsigned char)s[d] > louds_pt.labels[i]){ //seleziona come blocco quello più a destra nel subtrie

                    while(louds_pt.louds[start] == 1){
                        //take always the last arc of the node
                        index = louds_pt.get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 

                        (*tot_visited_nodes)++; 
                        num_nodes.push_back(start);
                    }
                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - louds_pt.rank_10(start); 

                    auto off = louds_pt.values[nleaves-1];

                    accessed_pages_indexes.push_back(off);

                    res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                    if(res_mincore == -1){
                        std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                        std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                    }
                    if((int)(vec[0] & 1) == 0){
                        (*fault_page)++;
                    }
                    (*access_to_pages)++;
                    (*random_access_to_pages)++;

                    return mmap_space.scan_block_with_lcp(s, d, off * logical_block_size, blocks_rank[off+1]-blocks_rank[off], blocks_rank[off]); 
                }
                
                if(first_char < 0){ 
                    start = seen_arcs[--seen]; 

                    (*tot_visited_nodes)++; 
                    num_nodes.push_back(start);

                    index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                
                    auto off = louds_pt.values[nleaves];

                    accessed_pages_indexes.push_back(off);

                    res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                    if(res_mincore == -1){
                        std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                        std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                    }
                    if((int)(vec[0] & 1) == 0){
                        (*fault_page)++;
                    }
                    (*access_to_pages)++;
                    (*random_access_to_pages)++;

                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 

                }
                
                //s[d] == labels[i] and long edges are in cache
                lcp_cache += first_char; //add 1 because s[d] == labels[i] 

                if(louds_pt.louds[start] == 0){
                    break;
                }
                
                //for sure louds_pt.louds[start] == 1
                child_len = louds_pt.lengths[louds_pt.rank_10(start)];

                if(child_len > 1){

                    if(isInCache[i] == 0){ 
                        path_in_cache = false;
                        break;
                    }

                    //else: is in cache
                    char * edge_s = &text_edges[text_pointer[rank_1_cache(i)]]; //does not count the current 1
                    int j = 0;

                    int compare;
                
                    while(path_in_cache && d+j+1 < s_len && (edge_s + j)[0]!='\0'){
                        compare = (unsigned char)s[d+j+1] - (unsigned char)((edge_s + j)[0]);
                        path_in_cache = ( compare == 0 ); //1 (true) if they are equal; 0(false) otherwise
                        j += path_in_cache; //if they are equal path_in_cache=1 so increase j, otherwise j does not change
                    }
                    lcp_cache += j;

                    if(compare > 0){//((unsigned char)s[d+j+1] > (unsigned char)(edge_s + j)[0]){ //seleziona come blocco quello più a destra nel subtrie

                        while(louds_pt.louds[start] == 1){
                            //take always the last arc of the node
                            index = louds_pt.get_end_zero(start) - rank;
                            start = select_0.selectZero(index - 1)+1;
                            rank = index; 

                            (*tot_visited_nodes)++; 
                            num_nodes.push_back(start);
                        }
                        //here louds[start] == 0
                        rank++; 
                        nleaves = rank - louds_pt.rank_10(start); 

                        auto off = louds_pt.values[nleaves-1];

                        accessed_pages_indexes.push_back(off);

                        res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                        if(res_mincore == -1){
                            std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                            std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                        }
                        if((int)(vec[0] & 1) == 0){
                            (*fault_page)++;
                        }
                        (*access_to_pages)++;
                        (*random_access_to_pages)++;

                        return mmap_space.scan_block_with_lcp(s, lcp_cache, off * logical_block_size, blocks_rank[off+1]-blocks_rank[off], blocks_rank[off]); 
                    }
                    if(compare < 0){ 
                        start = seen_arcs[--seen]; 

                        (*tot_visited_nodes)++; 
                        num_nodes.push_back(start);

                        index = louds_pt.get_left_block(&start, rank, select_0, seen_arcs, seen);
                        if(index == -1)
                            return -1;
            
                        rank = index;
                        nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                    
                        auto off = louds_pt.values[nleaves];

                        accessed_pages_indexes.push_back(off);

                        res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                        if(res_mincore == -1){
                            std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                            std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                        }
                        if((int)(vec[0] & 1) == 0){
                            (*fault_page)++;
                        }
                        (*access_to_pages)++;
                        (*random_access_to_pages)++;

                        return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 

                    }
                }
            }
        }

        //if you execute the body of the while path_in_cache is false FOR SURE
        while(louds_pt.louds[start] == 1){

            d += louds_pt.lengths[louds_pt.rank_10(start)]; 
            
            if(s_len < d){
                //take the left-most leaf in the current subtree
                seen_arcs[seen++] = start;
                start = select_0.selectZero(index)+1;
                rank = index+1;
                index = start - rank;  //rank1

                (*tot_visited_nodes)++; 
                num_nodes.push_back(start);
            }
            else{   

                i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node
                rank = i+1; 
                index = start - rank;

                (*tot_visited_nodes)++; 
                num_nodes.push_back(start);

            }
        }
        //here louds[start] == 0 
    
        //the current node is a leaf (a block is identified)  
        nleaves = rank - louds_pt.rank_10(start);

        if(path_in_cache && s_len == d && louds_pt.labels[i] =='\0'){
            return blocks_rank[louds_pt.values[nleaves]];
        }

        accessed_pages_indexes.push_back(louds_pt.values[nleaves]);

        res_mincore = mincore(mmapped_content + louds_pt.values[nleaves] * logical_block_size, page_size, vec);
        if(res_mincore == -1){
            std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
            std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
        }
        if((int)(vec[0] & 1) == 0){
            (*fault_page)++;
        }
        (*access_to_pages)++;
        (*random_access_to_pages)++;

        size_t first_off = louds_pt.values[nleaves];

        //access to the first string of the block
        s_block = mmap_space.access_at(louds_pt.values[nleaves] * logical_block_size);

        //compute where is the mismatch between the string of the block and the searched one
        int lcp = lcp_cache; //the first lcp_cache are equal
        while (s[lcp] != '\0' && s_block[lcp] != '\0' && s[lcp] == s_block[lcp])
            lcp++;

        auto len = strlen(s_block);
        
        if(lcp == s_len && s_len == len){ //string found
            return blocks_rank[louds_pt.values[nleaves]] +1;
        }

        bool compare = (unsigned char)s[lcp] < (unsigned char)s_block[lcp]; 

        if(lcp > d && !compare){ //scan the current block (s > s_block)
            auto off = louds_pt.values[nleaves]; 

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

        if(path_in_cache){ //scan the previous block (s < s_block)
            auto off = louds_pt.values[nleaves] -1; 

            accessed_pages_indexes.push_back(off);

            res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
            if(res_mincore == -1){
                std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
            }
            if((int)(vec[0] & 1) == 0){
                (*fault_page)++;
            }
            (*access_to_pages)++;

            return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 
        }

        int x;
        
        while(true){

            x = rank;
            //compute the position of the current node in the parent node encoding
            start = seen_arcs[--seen]; 
            rank = start - x + 1;    

            (*tot_visited_nodes)++; 
            num_nodes.push_back(start);

            if(lcp < d){ //d (depth) is always > 0
                d -= louds_pt.lengths[louds_pt.rank_10(start)]; //compute the length at the parent node
            }
            else {
                if(compare){

                    index = louds_pt.get_left_block(&start, x, select_0, seen_arcs, seen);
                    if(index == -1)
                        return -1;
        
                    rank = index;
                    nleaves = rank - louds_pt.rank_10(start); // count also the current node that is a leaf
                
                    auto off = louds_pt.values[nleaves];

                    accessed_pages_indexes.push_back(off);

                    res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                    if(res_mincore == -1){
                        std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                        std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                    }
                    if((int)(vec[0] & 1) == 0){
                        (*fault_page)++;
                    }
                    (*access_to_pages)++;

                    if(abs(int(first_off - off)) > 1){
                        (*random_access_to_pages)++;
                    }

                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1] - blocks_rank[off], blocks_rank[off]); 
                }
                else{
                    index = x - 1;  
                    i = louds_pt.low_linear_search(index, start, (unsigned char) s[d]); 

                    start = select_0.selectZero(i)+1;
                    rank = i+1; 

                    (*tot_visited_nodes)++; 
                    num_nodes.push_back(start);

                    //take the block to the extremely right in the current subtrie
                    while(louds_pt.louds[start] == 1){
                        //take always the last arc of the node
                        index = louds_pt.get_end_zero(start) - rank;
                        start = select_0.selectZero(index - 1)+1;
                        rank = index; 

                        (*tot_visited_nodes)++; 
                        num_nodes.push_back(start);
                    }

                    //here louds[start] == 0
                    rank++; 
                    nleaves = rank - louds_pt.rank_10(start); 

                    auto off = louds_pt.values[nleaves-1];

                    accessed_pages_indexes.push_back(off);

                    res_mincore = mincore(mmapped_content + off * logical_block_size, page_size, vec);
                    if(res_mincore == -1){
                        std::cout<<"ERROR IN THE MINCORE FUNCTION"<<std::endl;
                        std::cout << "mincore error failed: " << std::strerror(errno) << '\n';
                    }
                    if((int)(vec[0] & 1) == 0){
                        (*fault_page)++;
                    }
                    (*access_to_pages)++;

                    if(abs(int(first_off - off)) > 1){
                        (*random_access_to_pages)++;
                    }

                    return mmap_space.scan_block(s, off * logical_block_size, blocks_rank[off+1]-blocks_rank[off], blocks_rank[off]); 
                }
            }
        } 
    }




};
