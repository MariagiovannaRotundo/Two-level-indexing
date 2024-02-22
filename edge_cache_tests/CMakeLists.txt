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


add_executable(generate_test_caching generate_test_caching.cpp)
target_link_libraries(generate_test_caching divsufsort sdsl divsufsort64)

















