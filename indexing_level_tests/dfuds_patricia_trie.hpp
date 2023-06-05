#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <stdio.h>
#include <algorithm>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/vectors.hpp>
#include <sdsl/bp_support.hpp>
#include <math.h>
#include <sux/bits/SimpleSelectZeroHalf.hpp>


//struct used for the construction of the DFUDS sequence
struct dfs_node{
    int i;
    int j;
    int d;
    int d_parent;
};



class DFUDS_PatriciaTrie {    
    public:   


    sdsl::bit_vector dfuds; //DFUDS sequence
    std::vector<unsigned char> labels; //labels on the Patricia Trie
    sdsl::int_vector<> lengths;
    sdsl::rank_support_v<10, 2> rank_10; 

    sdsl::bp_support_sada<> bps; // support of Balanced Parenthesis. Used for the rank1 and find_close operations
    size_t pt_heigth;


    //create the succinct Patricia trie with the DFUDS sequence
    // - input: the dataset to use to build the Patricia Trie
    void build_trie(std::vector<std::string>& wordList){
        
        sdsl::bit_vector dfuds_tmp = sdsl::bit_vector(4 * wordList.size(), 0);
        sdsl::int_vector<> lengths_tmp(wordList.size());

        //the first 3 bits of DFUDS are 110
        dfuds_tmp[0] = 1;
        dfuds_tmp[1] = 1;
        dfuds_tmp[2] = 0;

        size_t dfuds_pos = 3;       //positions 0, 1 and 2 of DFUDS are already set
        int lenght_pos = 0;
        std::vector<dfs_node> dfs_visit; //vector used as stack to follow a DFUDS order in visiting nodes

        //create the root
        dfs_node root;
        root.i = 0;                    //first string of the "considered subtrie"
        root.j = (int)wordList.size(); //last string of of the "considered subtrie"
        root.d = 0;                    //depth of the node
        root.d_parent = 0;             //depth of the parent node

        dfs_visit.push_back(root);

        while(!dfs_visit.empty()){

            std::vector<int> indexes;        //vector of position where the label of the subtree starts
            std::queue<int> level_labels;   //vector with the labels that appear in the level in the subtrie

            dfs_node node = dfs_visit.back(); //read the last node to simulate a stack

            if(node.i>=node.j){ 
                std::cout<<"Error "<<std::endl;
                break;
            }
            if(node.j-node.i==1){ //the node has not children but it is a leaf
                dfuds_pos++; //by default in dfuds[dfuds_pos] there is 0
                dfs_visit.pop_back();  
                continue;
            }

            char last = wordList[node.i][node.d]; //get the letter in position d in the first word

            indexes.push_back(node.i);
            level_labels.push(last);
            for(int k=node.i+1; k<node.j; k++){
                if(wordList[k][node.d] != last){
                    last = wordList[k][node.d];
                    indexes.push_back(k);
                    level_labels.push(last);
                }
            }
            indexes.push_back(node.j);

            //if for all strings the d-th caracter is the same
            if(level_labels.size() == 1){
                if(node.d==0){//we are considering the root node 
                    dfuds_tmp[dfuds_pos] = 1;
                    dfuds_pos+=2;

                    lengths_tmp[lenght_pos++] = 0;
                    labels.push_back(level_labels.front());

                    dfs_visit.back().d_parent = 0;
                    dfs_visit.back().d += 1;
                    
                }
                else{//count the same node later because the tree is compressed
                    dfs_visit.back().d += 1;
                } 
            }
            else{//look at all the subtries 
                dfs_visit.pop_back();      
                lengths_tmp[lenght_pos++] = node.d - node.d_parent;

                while(!level_labels.empty()){

                    dfuds_tmp[dfuds_pos++] = 1;

                    labels.push_back(level_labels.front());
                    level_labels.pop();

                    //for each subtrie
                    dfs_node child;
                    child.j = indexes.back();
                    indexes.pop_back();
                    child.i = indexes.back();
                    child.d_parent = node.d;
                    child.d = node.d+1;
                    dfs_visit.push_back(child);
                }
                dfuds_pos++;//set the 0 as last bit for that node
            }
        }


        lengths_tmp.resize(lenght_pos);
        dfuds_tmp.resize(dfuds_pos);
        sdsl::util::bit_compress(lengths_tmp);

        dfuds = dfuds_tmp;
        lengths = lengths_tmp;

        sdsl::bp_support_sada<> bps_tmp(&dfuds);
        sdsl::rank_support_v<10, 2> rank_10_temp(&dfuds);

        bps = bps_tmp;
        rank_10 = rank_10_temp;
    }



    //to compute the heigth of the patricia trie
    //DO NOT USE DIRECTLY THIS FUNCTION BUT BY THE FUNCTION compute_heigth
    size_t height(std::vector<std::string>& wordList, size_t i, size_t j, size_t d){

        if(i>=j)
            return -1;

        //there is only one string in the range 
        if(j-i==1) 
            return 0;
        
        size_t h = 0;
        std::queue<size_t> indexes;
        char last = wordList[i][d]; //get the letter in position d in the i-th word

        //localize the subtries
        indexes.push(i);
        for(size_t k=i+1; k<j; k++){
            if(wordList[k][d] != last){
                last = wordList[k][d];
                indexes.push(k);
            }
        }
        indexes.push(j);

        //if for all strings the d-th caracter is the same
        if(indexes.size() == 2){
            if(d==0){//we are considering the root node in the height
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


    //compute the height of patricia tree
    // - input: the vector with strings used to build the Patricia Trie
    void compute_heigth(std::vector<std::string>& wordList) {
        pt_heigth = height(wordList, 0, wordList.size(), 0);
        std::cout << "\nheight of the trie: " << pt_heigth << std::endl;
    }


    //return the height of patricia tree
    size_t get_heigth() {
        return pt_heigth;
    }


    //return the size in bytes of the patricia trie
    size_t getSize(){

        size_t size = 0;
        
        size += sdsl::size_in_bytes(dfuds);
        size += labels.size() * sizeof(char);
        size += sdsl::size_in_bytes(lengths);
        size += sdsl::size_in_bytes(rank_10);
        size += sdsl::size_in_bytes(bps);

        return size;
    }



    //used in the traversal of the Patricia trie to implement the succ_0(x) operation
    // - input: the position x on which compute the succ_0
    inline int compute_succ_0(int start){
        while(dfuds[start] == 1){ 
            start++;
        }
        return start;
    }



    //return the number of the searched child by counting the children from 0
    // - input: the current position; the label to search; the variable where to write the position of the 
    // first 0 that follows the currect position
    inline size_t search_child(size_t start, unsigned char s, size_t * succ_0){
        
        auto index = bps.rank(start-1) - 2; //minus 2 because we have the 110 sequence at start of dfuds

        int pos = index; //rank_1 to count the 1s until the current position
        int i = index + 1; //pick the first label as minimum and start the analysis from the second label
        while(dfuds[++start] == 1){//++start because we start from the 1 corresponding to the 2nd label
            pos += (labels[i] <= s);
            i++;
        }
        (*succ_0) = start;
        return pos - index;
    }



    
    //look for the string s by traversing the Patricia Trie
    // - input: the string s; the vector used as stack to upward traverse the trie;
    // the stack used to store the argument used in the find_cllse operations during the downward 
    // traversal (used in the upward traversal) ; the vector with the visited nodes
    size_t lookup_no_strings(const std::string &s, std::vector<size_t>& seen_arcs, 
            std::vector<size_t>& arg_close, std::vector<size_t>& seen_nodes){

        int start = 3; //position in which a node starts in dfuds encoding (skip the initial 110)
        size_t d = 0; //depth in the string s and in the trie
        size_t nleaves, n_child;
        seen_arcs.clear();
        arg_close.clear();
        seen_nodes.clear();
        int seen = 0;
        size_t succ_0 = 0;


        //if the tree has ony one block: dfuds = 1100
        if(dfuds.size() == 4){
           return 1;
        }
        
        //downward traversal
        while(dfuds[start] == 1){
            //depth in the Patricia trie
            d += lengths[rank_10(start)-1]; //-1 because rank_10 counts a node more since the initial sequence 110
            
            if(s.length() < d){
                //take the left-most child 
                seen_arcs[seen] = start;
                seen_nodes[seen] = start;
                auto end_pos = compute_succ_0(start);
                start = bps.find_close(end_pos - 1) + 1;
                arg_close[seen++] = end_pos - 1;
            
            }
            else{   
                n_child = search_child(start, (unsigned char) s[d], &succ_0);
                seen_nodes[seen] = start;
                seen_arcs[seen] = start + n_child;
                arg_close[seen++] = succ_0 - n_child -1;
                start = bps.find_close(succ_0 - n_child -1) + 1; //- 1 because here the children must be counted from 1 but we count from 0
            }
        }

       
        //the current node is a leaf
        //rank_0(i) = i - rank_1(i) ----> i - rank_1(i) - rank_10 
        nleaves = start - bps.rank(start) - rank_10(start);
       
        size_t lcp = 0;

        while(true){

            //position of the current node in the parent node encoding
            start = seen_arcs[--seen]; 

            if(lcp < d){
                //go upward on the trie
                d -= lengths[rank_10(start) - 1]; //compute the length of root-to-node path at the parent node
            }
            else {
                return 1;
            }
        } 
    }

};
