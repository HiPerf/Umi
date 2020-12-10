find_package (Threads REQUIRED)

set(DOCOS_CORE_SOURCES 
    common/result_of.hpp
    common/tao.hpp
    common/types.hpp
    containers/concepts.hpp
    containers/dictionary.hpp
    containers/pool_item.hpp
    containers/pooled_static_vector.hpp
    containers/static_store.hpp
    containers/store.hpp
    containers/thread_local_tasks.cpp
    containers/thread_local_tasks.hpp
    containers/ticket.hpp
    entity/entity.hpp
    entity/scheme.hpp
    fiber/exclusive_work_stealing.hpp
    fiber/exclusive_work_stealing_impl.hpp
    fiber/exclusive_shared_work.hpp
    fiber/exclusive_shared_work_impl.hpp
    fiber/yield.hpp
    fiber/detail/yield.hpp
    ids/generator.hpp
    io/memmap.hpp
    io/memmap.cpp
    pools/plain_pool.hpp
    pools/singleton_pool.hpp
    pools/thread_local_pool.hpp
    traits/base_dic.hpp
    traits/contains.hpp
    traits/has_type.hpp
    traits/shared_function.hpp
    traits/tuple.hpp
    traits/without_duplicates.hpp
    updater/executor_registry.hpp
    updater/executor.hpp
    updater/updater.hpp
    docos.cpp)

if (INCLUDE_MONGODB_DRIVER)
    set(DOCOS_CORE_SOURCES 
        ${DOCOS_CORE_SOURCES}
        database/database.hpp
        database/database.cpp
        database/transaction.hpp
        database/transaction.cpp)
endif()

add_library(docos_core STATIC ${DOCOS_CORE_SOURCES})

target_link_libraries(docos_core PUBLIC Threads::Threads)
target_compile_features(docos_core PUBLIC cxx_std_20)
target_include_directories(docos_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(docos_core PUBLIC -stdlib=libc++)
    target_link_options(docos_core INTERFACE -stdlib=libc++ -lc++abi -lc++)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(docos_core PUBLIC _UNICODE UNICODE)
    target_compile_options(docos_core PUBLIC /MP /Zc:__cplusplus)
endif()
