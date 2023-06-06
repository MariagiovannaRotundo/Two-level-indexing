cmake_minimum_required (VERSION 3.12)
project (two_level_approach)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -O3 -pthread -g" )


add_subdirectory(indexing_level_tests)
add_subdirectory(two_level_tests)





