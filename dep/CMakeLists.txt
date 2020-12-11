include(FetchContent)


find_package(Boost ${BOOST_VERSION} REQUIRED COMPONENTS fiber)
target_include_directories(umi_core_lib PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(umi_core_lib PUBLIC ${Boost_LIBRARY_DIRS})
target_link_libraries(umi_core_lib PUBLIC Boost::fiber)


set(TAOCPP_TUPLE_BUILD_TESTS OFF CACHE INTERNAL "")

FetchContent_Declare(
    tuple
    GIT_REPOSITORY https://github.com/gpascualg/tuple
    GIT_TAG        master
)

FetchContent_GetProperties(tuple)
if(NOT tuple_POPULATED)
    FetchContent_Populate(tuple)
    add_subdirectory(${tuple_SOURCE_DIR} ${tuple_BINARY_DIR})
endif()

target_link_libraries(umi_core_lib PUBLIC tuple)



FetchContent_Declare(
    function
    GIT_REPOSITORY https://github.com/Naios/function2
    GIT_TAG        master
)

FetchContent_GetProperties(function)
if(NOT function_POPULATED)
    FetchContent_Populate(function)
    add_subdirectory(${function_SOURCE_DIR} ${function_BINARY_DIR})
endif()

target_link_libraries(umi_core_lib PUBLIC function2)

