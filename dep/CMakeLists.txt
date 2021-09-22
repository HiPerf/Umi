include(FetchContent)


find_package(Boost ${BOOST_VERSION} REQUIRED COMPONENTS fiber)
target_include_directories(umi_core_lib SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
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

target_link_libraries_system(umi_core_lib PUBLIC tuple)



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

target_link_libraries_system(umi_core_lib PUBLIC function2)



FetchContent_Declare(
    any_ptr
    GIT_REPOSITORY https://github.com/scottslacksmith/any_ptr
    GIT_TAG        master
)

FetchContent_GetProperties(any_ptr)
if(NOT any_ptr_POPULATED)
    FetchContent_Populate(any_ptr)
endif()

target_include_directories(umi_core_lib SYSTEM PUBLIC ${CMAKE_BINARY_DIR}/_deps/any_ptr-src/include)



FetchContent_Declare(
    ctti_dep
    GIT_REPOSITORY https://github.com/Manu343726/ctti
    GIT_TAG        master
)

FetchContent_GetProperties(ctti_dep)
if(NOT ctti_dep_POPULATED)
    FetchContent_Populate(ctti_dep)
    add_subdirectory(${ctti_dep_SOURCE_DIR} ${ctti_dep_BINARY_DIR})
endif()

target_link_libraries_system(umi_core_lib PUBLIC ctti)




FetchContent_Declare(
    range_v3_dep
    GIT_REPOSITORY https://github.com/ericniebler/range-v3
    GIT_TAG        master
)

FetchContent_GetProperties(range_v3_dep)
if(NOT range_v3_dep_POPULATED)
    FetchContent_Populate(range_v3_dep)
    add_subdirectory(${range_v3_dep_SOURCE_DIR} ${range_v3_dep_BINARY_DIR})
endif()

target_link_libraries_system(umi_core_lib PUBLIC range-v3)




# ---------------------------------
# ########### SPDLOG #############
# ---------------------------------
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        v1.9.2
)

FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()

target_link_libraries(umi_core_lib PUBLIC spdlog)
