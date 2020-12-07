add_executable(docos_test 
    test_scheme.cpp)

target_link_libraries(docos_test PRIVATE docos_core)
target_compile_features(docos_test PRIVATE cxx_std_20)
target_include_directories(docos_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
