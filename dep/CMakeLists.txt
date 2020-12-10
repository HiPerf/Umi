include(FetchContent)


find_package(Boost ${BOOST_VERSION} REQUIRED COMPONENTS fiber)
target_include_directories(docos_core PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(docos_core PUBLIC ${Boost_LIBRARY_DIRS})
target_link_libraries(docos_core PUBLIC Boost::fiber)


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

target_link_libraries(docos_core PUBLIC tuple)



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

target_link_libraries(docos_core PUBLIC function2)



if (INCLUDE_MONGODB_DRIVER)
    find_package(Git REQUIRED)

    # ---------------------------------
    # ############ MONGOC #############
    # ---------------------------------
    set(ENABLE_SASL             "OFF" CACHE INTERNAL "")
    set(ENABLE_SSL              "OFF" CACHE INTERNAL "")
    set(ENABLE_TESTS            OFF CACHE INTERNAL "")
    set(ENABLE_EXAMPLES         OFF CACHE INTERNAL "")

    FetchContent_Declare(
        mongoc
        GIT_REPOSITORY https://github.com/mongodb/mongo-c-driver
        GIT_TAG        1.17.3
    )

    FetchContent_GetProperties(mongoc)
    if(NOT mongoc_POPULATED)
        FetchContent_Populate(mongoc)
        add_subdirectory(${mongoc_SOURCE_DIR} ${mongoc_BINARY_DIR})
    endif()

    target_link_libraries(docos_core PUBLIC mongoc_static)
    # target_include_directories(docos_core PRIVATE ${mongoc_SOURCE_DIR}/src)
    # target_include_directories(docos_core PRIVATE ${mongoc_BINARY_DIR}/src)


    # ---------------------------------
    # ########## MONGOCXX #############
    # ---------------------------------
    set(ENABLE_SASL                 OFF CACHE INTERNAL "")
    set(ENABLE_SSL                  OFF CACHE INTERNAL "")
    set(ENABLE_TESTS                OFF CACHE INTERNAL "")
    set(ENABLE_EXAMPLES             OFF CACHE INTERNAL "")
    set(BUILD_SHARED_LIBS           OFF CACHE INTERNAL "")
    set(LIBBSON_FROM_SUBPROJECT     ${CMAKE_BINARY_DIR}/_deps/mongoc-src/src/libbson)
    set(LIBBSON_BINARY_DIR          ${CMAKE_BINARY_DIR}/_deps/mongoc-build/src/libbson)
    set(LIBMONGOC_FROM_SUBPROJECT   ${CMAKE_BINARY_DIR}/_deps/mongoc-src/src/libmongoc)
    set(LIBMONGOC_BINARY_DIR        ${CMAKE_BINARY_DIR}/_deps/mongoc-build/src/libmongoc)

    FetchContent_Declare(
        mongocxx
        GIT_REPOSITORY https://github.com/mongodb/mongo-cxx-driver
        GIT_TAG        r3.6.2
    )

    FetchContent_GetProperties(mongocxx)
    if(NOT mongocxx_POPULATED)
        FetchContent_Populate(mongocxx)
        
        execute_process(COMMAND ${GIT_EXECUTABLE} stash
                            WORKING_DIRECTORY ${mongocxx_SOURCE_DIR}
                            RESULT_VARIABLE GIT_RESULT)

        execute_process(COMMAND ${GIT_EXECUTABLE} apply --reject --whitespace=fix ${CMAKE_SOURCE_DIR}/dep/mongocxx.patch
                            WORKING_DIRECTORY ${mongocxx_SOURCE_DIR}
                            RESULT_VARIABLE GIT_RESULT)

        add_subdirectory(${mongocxx_SOURCE_DIR} ${mongocxx_BINARY_DIR})
    endif()

    target_link_libraries(docos_core PUBLIC mongocxx_static)
    target_include_directories(docos_core PUBLIC ${LIBBSON_FROM_SUBPROJECT}/src)
    target_include_directories(docos_core PUBLIC ${LIBBSON_BINARY_DIR}/src)
    target_include_directories(docos_core PUBLIC ${LIBMONGOC_FROM_SUBPROJECT}/src)
    target_include_directories(docos_core PUBLIC ${LIBMONGOC_BINARY_DIR}/src)
    target_include_directories(docos_core PUBLIC ${mongocxx_SOURCE_DIR}/src)
    target_include_directories(docos_core PUBLIC ${mongocxx_BINARY_DIR}/src)

endif()
