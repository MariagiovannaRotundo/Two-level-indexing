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


//struct used in the construction of the LOUDS sequence
struct bfs_node{
    int i;
    int j;
    int d;
    int d_parent;
};


class LOUDS_PatriciaTrie {  
    public:   


    sdsl::bit_vector louds; //LOUDS sequnce
    std::vector<char> labels;
    sdsl::int_vector<> lengths;
    sdsl::int_vector<> values;
    sdsl::rank_support_v<10, 2> rank_10; 
   


    //create the succinct Patricia trie with the LOUDS sequence
    // - input: the dataset to use to build the Patricia Trie
    void build_trie(std::vector<std::string>& wordList){
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
        b.d = 0;
        b.d_parent = 0;

        std::queue<bfs_node> bfs_visit; //to visit the "nodes" in LOUDS order
        bfs_visit.push(b);

        int p = 0; //position in louds_tmp bit vector
        int v = 0;
        int l = 0;

        while(!bfs_visit.empty()){

            std::queue<int> indexes;
            std::queue<int> level_labels;

            bfs_node b = bfs_visit.front();

            if(b.i>=b.j){ 
                std::cout<<"Error "<<std::endl;
                break;
            }
            if(b.j-b.i==1){ //the node has not children but is a leaf
                values_tmp[v++] = b.i;
                p++; //by default in louds_tmp[p] = 0
                bfs_visit.pop();  
                continue;
            }

            char last = wordList[b.i][b.d]; 

            //localize the subtries starting in the node we are examinating
            indexes.push(b.i);
            level_labels.push(last);
            for(int k=b.i+1; k<b.j; k++){
                if(wordList[k][b.d] != last){
                    last = wordList[k][b.d];
                    indexes.push(k);
                    level_labels.push(last);
                }
            }
            indexes.push(b.j);

            //if for all strings the d-th caracter is the same
            if(level_labels.size() == 1){
                if(b.d==0){//we are considering the root node in the height
                    louds_tmp[p] = 1;
                    p+=2;

                    lengths_tmp[l++] = 0;
                    labels.push_back(level_labels.front());

                    bfs_visit.front().d_parent = 0;
                    bfs_visit.front().d += 1;
                    
                }
                else{//count the same node later because the tree is compressed
                    bfs_visit.front().d += 1;
                } 
            }
            else{//look at all the subtries 
                bfs_visit.pop();   
                lengths_tmp[l++] = b.d - b.d_parent;

                while(!level_labels.empty()){

                    louds_tmp[p++] = 1;

                    labels.push_back(level_labels.front());
                    level_labels.pop();

                    //for each subtrie
                    bfs_node bc;
                    bc.i = indexes.front();
                    indexes.pop();
                    bc.j = indexes.front();
                    bc.d_parent = b.d;
                    bc.d = b.d+1;
                    bfs_visit.push(bc);
                }
                p++;//set the 0 as last bit for that node
            }
        }

        values_tmp.resize(v);
        lengths_tmp.resize(l);
        louds_tmp.resize(p);

        sdsl::util::bit_compress(values_tmp);  
        sdsl::util::bit_compress(lengths_tmp);  

        louds = louds_tmp;
        lengths = lengths_tmp;
        values = values_tmp;

        sdsl::rank_support_v<10, 2> rank_10_tmp(&louds);

        rank_10 = rank_10_tmp;
    }


    //to compute the heigth of the patricia trie
    //DO NOT USE DIRECTLY THIS FUNCTION BUT BY THE FUNCTION compute_heigth
    int height(std::vector<std::string>& wordList, int i, int j, int d){

        //error with the parameters
        if(i>=j)
            return -1;
        //only one string in the range 
        if(j-i==1) 
            return 0;
        
        int h = 0;
        std::queue<int> indexes;
        char last = wordList[i][d]; //get the letter in position d in the first word

        //localize the subtries
        indexes.push(i);
        for(int k=i+1; k<j; k++){
            if(wordList[k][d] != last){
                last = wordList[k][d];
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
                int first = indexes.front();
                indexes.pop();
                int hc = 1 + height(wordList, first, indexes.front(), d+1);
                if(hc > h) h = hc;
            }
        }

        return  h;
    }


    //compute the height of patricia tree
    // - input: the vector with strings used to build the Patricia Trie
    int get_heigth(std::vector<std::string>& wordList) {
        int h = height(wordList, 0, wordList.size(), 0);
        return h;
    }

    //return the LOUDS sequence
    sdsl::bit_vector getLouds(){
        return louds;
    }


    //returns the size in bytes of the patricia trie 
    // - input: the select_0 data structure
    size_t getSize_sux(sux::bits::SimpleSelectZeroHalf<>& select_0){

        size_t size = sdsl::size_in_bytes(louds);
        size += labels.size() * sizeof(char);
        size += sdsl::size_in_bytes(lengths);
        size += sdsl::size_in_bytes(values);
        size += select_0.bitCount() / 8; // bits -> bytes
        size += sdsl::size_in_bytes(rank_10);
    
        return size;
    }


    //search the character s in the labels that starts from index in the vector of label. start is the position in 
    // LOUDS of the label in labels[index].
    // - input: the index of the first label of the node in labels sequence; the position in LODUS;
    // the character to search
    inline int low_linear_search(int index, int start, char s){
    
        int pos = index; //position of the just smaller char respect to s char
        int i = index + 1; //pick the first label as minimum and start the analysis from the second label

        while(louds[++start] == 1){//++start because we start from the 1 corresponding to the 2nd arc (2nd label)
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


    //look for the string s by traversing the Patricia Trie
    // - input: the string s; the select_0 data scrtucter on the LOUDS sequence; 
    // the vector of nodes visited during the downward traversal
    int lookup_no_strings(const std::string &s, sux::bits::SimpleSelectZeroHalf<>& select_0, 
        std::vector<int>& seen_arcs){

        int start = 0; 
        int d = 0; //depth in the trie
        int nleaves, n, index, rank, i; 
        seen_arcs.clear();
        int seen = 0;

        //if the tree has ony one block
        if(louds.size() == 1){
            return 1; 
        }

        rank = 0;
        index = 0;

        //downward traversal
        while(louds[start] == 1){
            //depth in the Patricia trie
            d += lengths[rank_10(start)]; //length associated to the current node for the blind search
            
            if(s.length() < d){
                //take the left-most child 
                seen_arcs[seen++] = start;
                start = select_0.selectZero(index)+1;
                rank = index+1; //rank0(start)
                index = start - rank;   //rank1(start)
            }
            else{   
                i = low_linear_search(index, start, s[d]); 

                //index in louds of the chosen 1 (the chosen child)
                seen_arcs[seen++] = start + i - index;

                start = select_0.selectZero(i)+1; //i-th children node //i+1 to count the current 1
                rank = i+1; 
                //position where the current node starts in labels vector
                index = start - rank;
            }
        }
    
        //the current node is a leaf (a block is identified)  
        nleaves = rank - rank_10(start);

        int lcp = 0;
        int x;

        while(true){

            x = rank;
            //compute the position of the current node in the parent node encoding
            start = seen_arcs[--seen]; 
            rank = start - x + 1;    

            if(lcp < d){
                //go upward on the trie
                d -= lengths[rank_10(start)]; //compute the root-to-node path length at the parent node
            }
            else {
                //the root has been reached
                return 1;
            }
        } 
    }

};