add_subdirectory(src)
add_subdirectory(dep)

if (BUILD_TESTS)
    add_subdirectory(test)
endif()
