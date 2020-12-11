add_executable(umi_server_test 
    test_database_collisions.cpp)

target_link_libraries(umi_server_test PRIVATE umi_server_lib)
target_compile_features(umi_server_test PRIVATE cxx_std_20)
target_include_directories(umi_server_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(umi_server_test PRIVATE BOOST_ASIO_DISABLE_CONCEPTS)
    target_link_libraries(umi_server_test PRIVATE m)
endif()
