cmake_minimum_required (VERSION 3.10)
project (two_level_indexing)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -O3 -pthread -g -I ~/include" ) 

include_directories(  ~/include)
include_directories(  ~/lib)

link_directories(~/lib)

add_executable(generate_storage_zstd generate_storage_zstd.cpp)
target_link_libraries(generate_storage_zstd z divsufsort sdsl divsufsort64 zstd)

add_executable(decompress_times_zstd decompress_times_zstd.cpp)
target_link_libraries(decompress_times_zstd z divsufsort sdsl divsufsort64 zstd)


