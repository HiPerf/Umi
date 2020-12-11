include(FetchContent)

find_package (Python3 COMPONENTS Interpreter)
message("Using python ${Python3_EXECUTABLE}")


# We need Kumo now
FetchContent_Declare(
    kumo
    GIT_REPOSITORY https://github.com/AErbis/Kumo.git
    GIT_TAG        master
)

FetchContent_GetProperties(kumo)
if(NOT kumo_POPULATED)
    FetchContent_Populate(kumo)
endif()

set(KUMO_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo/config.json)
set(KUMO_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo/queues.kumo
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo/movement.kumo
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo/handshake.kumo
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo/login.kumo
    ${KUMO_CONFIG_FILE}
)
set(KUMO_OUTPUT_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/marshal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/marshal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/rpc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/rpc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/opcodes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/protocol_queues.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/structs.hpp
)

if (KUMO_GENERATE_CLIENT_FILES AND EXISTS "${KUMO_CLIENT_PATH}")
    set(KUMO_SOURCE_FILES
        ${KUMO_SOURCE_FILES}
        ${KUMO_CLIENT_CONFIG}
    )
    
    add_custom_command(
        OUTPUT   ${KUMO_OUTPUT_FILES}
        COMMAND  ${Python3_EXECUTABLE} 
            ${kumo_SOURCE_DIR}/run.py 
                --role server --dir ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo --out ${CMAKE_CURRENT_SOURCE_DIR}/kumo --lang kaminari-cpp --config ${KUMO_CONFIG_FILE}
                --role client --dir ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo --out ${KUMO_CLIENT_PATH} --lang ${KUMO_CLIENT_LANGUAGE} --config ${KUMO_CLIENT_CONFIG}
        DEPENDS  ${KUMO_SOURCE_FILES}
    )
else()
    add_custom_command(
        OUTPUT   ${KUMO_OUTPUT_FILES}
        COMMAND  ${Python3_EXECUTABLE} 
            --role server --dir ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo --out ${CMAKE_CURRENT_SOURCE_DIR}/kumo --lang kaminari-cpp --config ${KUMO_CONFIG_FILE}
        DEPENDS  ${KUMO_SOURCE_FILES}
    )
endif()

set(SERVER_SOURCES
    common/definitions.hpp
    core/client.hpp
    core/client.cpp
    core/handler.hpp
    core/handler.cpp
    core/server.hpp
    core/server.cpp    
    database/database.hpp
    database/database.cpp
    database/pseudorandom_unique_id.hpp
    database/pseudorandom_unique_id.cpp
    database/transaction.hpp
    database/transaction.cpp
    entities/transform.hpp
    entities/transform.cpp
    maps/cell.hpp
    maps/cell.cpp
    maps/map.hpp
    maps/map.cpp
    maps/offset.hpp
    maps/region.hpp
    maps/region.cpp
    ${KUMO_OUTPUT_FILES})

# LIBRARY
add_library(umi_server_lib STATIC ${SERVER_SOURCES})

target_include_directories(umi_server_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(umi_server_lib PUBLIC umi_core_lib)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(umi_server_lib PUBLIC BOOST_ASIO_DISABLE_CONCEPTS)
    target_link_libraries(umi_server_lib PUBLIC m)
endif()

# EXECUTABLE
add_executable(umi_server main.cpp)
target_link_libraries(umi_server PRIVATE umi_server_lib)

# RESOURCES
add_custom_command(
    TARGET umi_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            ${CMAKE_CURRENT_SOURCE_DIR}/../resources
            ${CMAKE_CURRENT_BINARY_DIR}/resources)
