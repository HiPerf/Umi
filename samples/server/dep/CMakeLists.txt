include(FetchContent)

find_package(Git REQUIRED)


FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        0.9.9.8
)

FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
    FetchContent_Populate(glm)
    add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})
endif()

target_link_libraries(umi_server_lib PUBLIC glm)



FetchContent_Declare(
    kaminari
    GIT_REPOSITORY https://github.com/AErbis/Kaminari-cpp.git
    GIT_TAG        master
)

FetchContent_GetProperties(kaminari)
if(NOT kaminari_POPULATED)
    FetchContent_Populate(kaminari)
    add_subdirectory(${kaminari_SOURCE_DIR} ${kaminari_BINARY_DIR})
endif()

target_link_libraries(umi_server_lib PUBLIC kaminari)


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

target_link_libraries(umi_server_lib PUBLIC mongoc_static)

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

    execute_process(COMMAND ${GIT_EXECUTABLE} apply --reject --whitespace=fix ${CMAKE_CURRENT_SOURCE_DIR}/mongocxx.patch
                        WORKING_DIRECTORY ${mongocxx_SOURCE_DIR}
                        RESULT_VARIABLE GIT_RESULT)

    add_subdirectory(${mongocxx_SOURCE_DIR} ${mongocxx_BINARY_DIR})
endif()

target_link_libraries(umi_server_lib PUBLIC mongocxx_static)
target_include_directories(umi_server_lib PUBLIC ${LIBBSON_FROM_SUBPROJECT}/src)
target_include_directories(umi_server_lib PUBLIC ${LIBBSON_BINARY_DIR}/src)
target_include_directories(umi_server_lib PUBLIC ${LIBMONGOC_FROM_SUBPROJECT}/src)
target_include_directories(umi_server_lib PUBLIC ${LIBMONGOC_BINARY_DIR}/src)
target_include_directories(umi_server_lib PUBLIC ${mongocxx_SOURCE_DIR}/src)
target_include_directories(umi_server_lib PUBLIC ${mongocxx_BINARY_DIR}/src)


# ---------------------------------
# ########### CRYPTOPP ############
# ---------------------------------
FetchContent_Declare(
    cryptopp
    GIT_REPOSITORY https://github.com/weidai11/cryptopp
    GIT_TAG        master
)

FetchContent_GetProperties(cryptopp)
if(NOT cryptopp_POPULATED)
    FetchContent_Populate(cryptopp)
    
    configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/cryptopp.cmake"
        ${cryptopp_SOURCE_DIR}/CMakeLists.txt
        COPYONLY
    )
    
    configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/cryptopp.config.cmake.in"
        ${cryptopp_SOURCE_DIR}/cryptopp-config.cmake
        COPYONLY
    )

    add_subdirectory(${cryptopp_SOURCE_DIR} ${cryptopp_BINARY_DIR})
endif()

target_link_libraries(umi_server_lib PUBLIC cryptopp-static)
