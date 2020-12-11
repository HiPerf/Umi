find_package (Threads REQUIRED)

set(CORE_SOURCES 
    async/async_executor.hpp
    async/async_executor.cpp
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
    updater/updater_all_async.hpp
    updater/updater_batched.hpp
    updater/updater_contiguous.hpp)

add_library(umi_core_lib STATIC ${CORE_SOURCES})

target_link_libraries(umi_core_lib PUBLIC Threads::Threads)
target_compile_features(umi_core_lib PUBLIC cxx_std_20)
target_include_directories(umi_core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(umi_core_lib PUBLIC -stdlib=libc++)
    target_link_options(umi_core_lib INTERFACE -stdlib=libc++ -lc++abi -lc++)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(umi_core_lib PUBLIC _UNICODE UNICODE)
    target_compile_options(umi_core_lib PUBLIC /MP /Zc:__cplusplus)
endif()
