#pragma once

#include <inttypes.h>
#include <tuple>

#include <containers/concepts/constructable.hpp>
#include <containers/concepts/destroyable.hpp>
#include <containers/concepts/has_scheme_created.hpp>
#include <containers/concepts/has_scheme_information.hpp>
#include <containers/concepts/has_sync.hpp>
#include <containers/concepts/has_update.hpp>
#include <containers/concepts/scheme_destroyable.hpp>


template<typename T, typename... Args>
concept base_constructable = requires { (void (T::*)(Args...)) &T::construct; };

template<typename T, typename... Args>
concept base_destroyable = requires { (void (T::*)(Args...)) &T::destroy; };

template<typename T>
concept poolable = base_constructable<T, uint64_t> && base_destroyable<T>;
