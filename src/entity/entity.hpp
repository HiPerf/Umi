#pragma once

#include "common/types.hpp"
#include "containers/pool_item.hpp"
#include "containers/dictionary.hpp"
#include "entity/dynamic_content.hpp"
#include "entity/scheme.hpp"

#include <any_ptr.h>


template <typename T>
class entity : public pool_item<entity<T>>
{
    template <typename... vectors> friend struct scheme;
    template <typename D, typename E, uint16_t I, typename R> friend class pooled_static_vector;
    template <typename D, typename... types> friend class updater;
    template <typename... types> friend class updater_batched;
    template <typename... types> friend class updater_contiguous;
    template <typename... types> friend class updater_all_async;
    template <typename D> friend class base_executor;

public:
    using derived_t = T;

public:
    inline entity_id_t id();

    template <typename... Args>
    static inline constexpr bool has_update()
    {
        return ::has_update<derived_t, Args...>;
    }

    template <typename... Args>
    static inline constexpr bool has_sync()
    {
        return ::has_sync<derived_t, Args...>;
    }

    inline entity<derived_t>* base()
    {
        return this;
    }

    inline derived_t* derived()
    {
        return reinterpret_cast<derived_t*> (this);
    }

    template <typename S>
    inline std::decay_t<S>* scheme() const
    {
        return xxx::any_ptr_cast<std::decay_t<S>>(_scheme);
    }


private:
    template <typename... Args>
    constexpr inline void update(Args&&... args);

    template <typename... Args>
    constexpr inline void sync(Args&&... args);

    template <typename... Args>
    constexpr inline void construct(entity_id_t id, Args&&... args);

    template <typename... Args>
    constexpr inline void destroy(Args&&... args);

    constexpr inline void scheme_created(dynamic_content&& entities);

    template <template <typename...> typename S, typename... components>
    constexpr inline void scheme_information(S<components...>& scheme);


    // inline 

private:
    entity_id_t _id;
    xxx::any_ptr _scheme;
    dynamic_content _entities;
};


template <typename derived_t>
inline entity_id_t entity<derived_t>::id()
{
    return _id;
}

template <typename derived_t>
template <typename... Args>
constexpr inline void entity<derived_t>::construct(entity_id_t id, Args&&... args)
{
    _id = id;

#if (__DEBUG__ || FORCE_ALL_CONSTRUCTORS) && !DISABLE_DEBUG_CONSTRUCTOR
    static_cast<derived_t&>(*this).construct(std::forward<Args>(args)...);
#else
    if constexpr (constructable<derived_t, Args...>)
    {
        static_cast<derived_t&>(*this).construct(std::forward<Args>(args)...);
    }
#endif
}

template <typename derived_t>
template <typename... Args>
constexpr inline void entity<derived_t>::destroy(Args&&... args)
{
    if constexpr (destroyable<derived_t, Args...>)
    {
        static_cast<derived_t&>(*this).destroy(std::forward<Args>(args)...);
    }
}

template <typename derived_t>
template <typename... Args>
constexpr inline void entity<derived_t>::update(Args&&... args)
{
    if constexpr (::has_update<derived_t, Args...>)
    {
        static_cast<derived_t&>(*this).update(std::forward<Args>(args)...);
    }
}

template <typename derived_t>
template <typename... Args>
constexpr inline void entity<derived_t>::sync(Args&&... args)
{
    if constexpr (::has_sync<derived_t, Args...>)
    {
        static_cast<derived_t&>(*this).sync(std::forward<Args>(args)...);
    }
}

template <typename derived_t>
constexpr inline void entity<derived_t>::scheme_created(dynamic_content&& entities)
{
    _entities = std::move(entities);

    if constexpr (has_scheme_created<derived_t>)
    {
        static_cast<derived_t&>(*this).scheme_created();
    }
}

template <typename derived_t>
template <template <typename...> typename S, typename... components>
constexpr inline void entity<derived_t>::scheme_information(S<components...>& scheme)
{
    _scheme = &scheme;

    if constexpr (has_scheme_information<derived_t, S, components...>)
    {
        static_cast<derived_t&>(*this).scheme_information(scheme);
    }
}
