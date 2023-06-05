#include <string>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <math.h>



//generate string queries from picking (pseudo) random string from the set of string word_block. For 
//the picking a seed is used to chose the strings. 
// - input: the set of strings from whom picking the strings; the percentage of strings to pick looking
//at the input set; the vector of strings where to write the string queries; the seed
void generate_query_dataset_seed(std::vector<std::string>& word_block, int percentage_size, 
        std::vector<std::string>& word_query, uint seed){

    srand(seed);

    //ordered index of chosen words
    std::set<int> words; 
    std::vector<int> unordered_words;

    //number of strings to pick
    int percentage = word_block.size() / 100 * percentage_size;

    //choose every string just once
    while(words.size()<percentage){
        int number = rand() % word_block.size();
        auto results = words.insert(number);
        if(results.second){
            unordered_words.push_back(number);
        }
    }
    
    //write the vector of string queries
    for(int j=0; j < unordered_words.size(); j++){
        word_query.push_back(word_block[unordered_words[j]]);
    }
    
}


//compute the LCP between two strings s1 and s2
// - input: the strings s1 and s2
int lcp(std::string s1, std::string s2){
    auto n = std::min(s1.length(), s2.length());
    for(int i=0; i<n; i++)
        if(s1[i] != s2[i])
            return i;
    return n;
}

//compute the distinguishing prefix of s2 by looking at s1 and s3
// - input: the strings s1, s2, s3
std::string trunc(std::string s1, std::string s2, std::string s3){
    auto m = std::max(lcp(s1, s2), lcp(s2, s3));
    m = std::min(s2.length(), (size_t)(m + 1));
    return s2.substr(0, m);
}


//read the words of the dataset stored in filename and for each string compute the distinguishing prefix
// - input: the name of the file containing the strings, the vector of strings where to store
//the distinguishing prefixes
void read_distinguishing(char* filename, std::vector<std::string>& wordList){

    
    std::ifstream ifs(filename, std::ifstream::in); //open in reading
    if (!ifs){
        std::cout<<"cannot open "<<filename<<"\n"<<std::endl;
        exit(0);
    }

    std::string word_pre = "";
    std::string word_curr;
    std::string word_next;


    //read the first the words
    while (getline(ifs, word_curr)){
        if (word_curr.size() > 0 && word_curr[word_curr.size()-1] == '\r'){
            word_curr = word_curr.substr(0, word_curr.size()-1);
        }
        if (word_curr == "") {
            continue;
        }
        else{
            break;
        }
    }

    //read the next string. If it is not present there is only one string in the dictionary
    if (!getline(ifs, word_next)){
        std::cout<<"ONLY 1 STRING IN THE DICTIONARY "<<std::endl;
        exit(0);
    }            

    do {
        if (word_next.size() > 0 && word_next[word_next.size()-1] == '\r'){
            word_next = word_next.substr(0, word_next.size()-1); 
        }
        if (word_next == "") {
            continue;
        }   

        //write the distinguishin prefix in the vector
        wordList.push_back(trunc(word_pre, word_curr, word_next));

        word_pre = word_curr;
        word_curr = word_next;

    } while (getline(ifs, word_next));

    wordList.push_back(trunc(word_pre, word_curr, ""));
}