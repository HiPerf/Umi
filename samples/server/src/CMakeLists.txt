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

set(KUMO_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo/queues.kumo
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo/movement.kumo
)
set(KUMO_OUTPUT_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/marshal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/marshal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/rpc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/rpc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/opcodes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/protocol_queues.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kumo/structs.hpp
)

add_custom_command(
    OUTPUT   ${KUMO_OUTPUT_FILES}
    COMMAND  ${Python3_EXECUTABLE} ${kumo_SOURCE_DIR}/run.py --role server --dir ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo --out ${CMAKE_CURRENT_SOURCE_DIR}/kumo --lang kaminari-cpp --config ${CMAKE_CURRENT_SOURCE_DIR}/../data/kumo/config.json
    DEPENDS  ${KUMO_SOURCE_FILES}
)

add_executable(docos_sample_server 
    common/definitions.hpp
    core/client.hpp
    core/client.cpp
    core/server.hpp
    core/server.cpp
    maps/map.hpp
    maps/map.cpp
    ${KUMO_OUTPUT_FILES}
    main.cpp)

target_include_directories(docos_sample_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(docos_sample_server PRIVATE docos_core)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(docos_sample_server PRIVATE BOOST_ASIO_DISABLE_CONCEPTS)
    target_link_libraries(docos_sample_server PRIVATE m)
endif()

add_custom_command(
    TARGET docos_sample_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            ${CMAKE_CURRENT_SOURCE_DIR}/../resources
            ${CMAKE_CURRENT_BINARY_DIR}/resources)
