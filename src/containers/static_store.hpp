#pragma once

#include "containers/store.hpp"

#include <unordered_map>


template <typename T, typename B>
class static_store : public store<T, B>
{
public:
    static inline static_store<T, B>* instance = nullptr;

public:
    static_store();
};

template <typename T, typename B>
static_store<T, B>::static_store()
{
    instance = this;
}

