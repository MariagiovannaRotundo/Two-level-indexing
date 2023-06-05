cmake_minimum_required (VERSION 3.10)
project (post_tesi)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -O3 -pthread -g" )


include_directories(~/lib/sux)


add_subdirectory(fast_succinct_trie)
add_subdirectory(CoCo-trie EXCLUDE_FROM_ALL)


add_executable(test_louds test_louds.cpp)
target_link_libraries(test_louds sdsl)

add_executable(test_dfuds test_dfuds.cpp)
target_link_libraries(test_dfuds sdsl)

add_executable(test_array test_array.cpp)
target_link_libraries(test_array sdsl)

add_executable(test_FST test_FST.cpp)
target_link_libraries(test_FST sdsl)


add_executable(test_PDT test_PDT.cpp)
target_include_directories(test_PDT PRIVATE path_decomposed_tries)
target_link_libraries(test_PDT succinct)

add_executable(generate_coco generate_coco.cpp)
target_include_directories(generate_coco PRIVATE CoCo-trie/include CoCo-trie/lib/ds2i)
target_link_libraries(generate_coco sdsl succinct  ${Boost_LIBRARIES})
target_compile_definitions(generate_coco PRIVATE BIG_ALPHABET)

add_executable(test_coco test_coco.cpp)
target_include_directories(test_coco PRIVATE CoCo-trie/include CoCo-trie/lib/ds2i)
target_link_libraries(test_coco sdsl succinct  ${Boost_LIBRARIES})
target_compile_definitions(test_coco PRIVATE BIG_ALPHABET)




