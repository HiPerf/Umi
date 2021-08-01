#pragma once

#include "containers/pool_item.hpp"

#include <inttypes.h>


template<typename T>
concept pool_item_derived = std::is_base_of_v<pool_item<T>, T>;


enum class storage_grow
{
    fixed           = 1,
    growable        = 2
};

enum class storage_layout
{
    continuous      = 1,
    partitioned     = 2
};

inline constexpr uint8_t storage_tag(storage_grow grow, storage_layout layout)
{
    return (grow << 4) | layout;
}
