set(CLIENT_SOURCES
    gx/camera/camera.hpp
    gx/camera/camera.cpp
    gx/mesh/mesh.hpp
    gx/mesh/mesh.cpp
    gx/shader/program.hpp
    gx/shader/program.cpp
    gx/transform/transform.hpp
    gl_safe.hpp)

add_library(umi_client_lib STATIC ${CLIENT_SOURCES})

target_include_directories(umi_client_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(umi_client_lib PUBLIC umi_core_lib)

add_executable(umi_client main.cpp)
target_link_libraries(umi_client PRIVATE umi_client_lib)

add_custom_command(
    TARGET umi_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            ${CMAKE_CURRENT_SOURCE_DIR}/../resources
            ${CMAKE_CURRENT_BINARY_DIR}/resources)
