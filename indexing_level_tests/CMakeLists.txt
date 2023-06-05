cmake_minimum_required (VERSION 3.10)
project (post_tesi)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -O3 -pthread -g -I ~/include" )


include_directories(  ~/include)
include_directories(  ~/lib)
include_directories(~/lib/sux)

link_directories(~/lib)

add_library(sux ~/lib/sux/sux/bits/SimpleSelectHalf.hpp)
set_target_properties(sux PROPERTIES LINKER_LANGUAGE CXX)


# add_executable(FST_on_sample FST_on_sample.cpp)
# target_link_libraries(FST_on_sample divsufsort sdsl divsufsort64)

add_executable(test_louds test_louds.cpp)
target_link_libraries(test_louds divsufsort sdsl divsufsort64)

add_executable(test_dfuds test_dfuds.cpp)
target_link_libraries(test_dfuds divsufsort sdsl divsufsort64)

add_executable(test_array test_array.cpp)
target_link_libraries(test_array divsufsort sdsl divsufsort64)




