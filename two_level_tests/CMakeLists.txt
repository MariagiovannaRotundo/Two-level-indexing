cmake_minimum_required (VERSION 3.10)
project (two_level_indexing)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -O3 -pthread -g -I ~/include" ) 

include_directories(  ~/include)
include_directories(  ~/lib)
include_directories(~/lib/sux)

link_directories(~/lib)


add_library(sux ~/lib/sux/sux/bits/SimpleSelectHalf.hpp)
set_target_properties(sux PROPERTIES LINKER_LANGUAGE CXX)


add_executable(generate_storage generate_storage.cpp)
target_link_libraries(generate_storage divsufsort sdsl divsufsort64)

add_executable(generate_query generate_query.cpp)
target_link_libraries(generate_query divsufsort sdsl divsufsort64)

add_executable(generate_test_PT generate_test_PT.cpp)
target_link_libraries(generate_test_PT divsufsort sdsl divsufsort64)

add_executable(generate_test_array generate_test_array.cpp)
target_link_libraries(generate_test_array divsufsort sdsl divsufsort64)

add_executable(save_first_strings save_first_strings.cpp)
target_link_libraries(save_first_strings divsufsort sdsl divsufsort64)














