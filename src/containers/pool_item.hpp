#pragma once

#include "containers/concepts.hpp"
#include "containers/ticket.hpp"



template <typename T>
class pool_item
{
public:
    pool_item() noexcept;
    pool_item(const pool_item&) = delete;
    pool_item(pool_item&& other) noexcept;
    pool_item& operator=(const pool_item&) = delete;
    pool_item& operator=(pool_item&& other) noexcept;

    inline bool has_ticket() const { return _ticket != nullptr; }
    inline typename ticket<T>::ptr ticket() const { return _ticket; }

    inline void refresh_ticket() noexcept;

protected:
    inline void recreate_ticket() noexcept;
    inline void invalidate_ticket() noexcept;
    void invalidate();

private:
   typename ::ticket<T>::ptr _ticket;
};

template <typename T>
pool_item<T>::pool_item() noexcept :
    _ticket(nullptr)
{}

template <typename T>
pool_item<T>::pool_item(pool_item&& other) noexcept :
    _ticket(std::move(other._ticket))
{
    refresh_ticket();
}

template <typename T>
pool_item<T>& pool_item<T>::operator=(pool_item&& other) noexcept
{
    _ticket = std::move(other._ticket);
    refresh_ticket();
    return *this;
}

template <typename T>
inline void pool_item<T>::recreate_ticket()  noexcept
{
    _ticket = typename ::ticket<T>::ptr(new ::ticket<T>(reinterpret_cast<T*>(this)));
}

template <typename T>
inline void pool_item<T>::refresh_ticket()  noexcept
{
    _ticket->_ptr = reinterpret_cast<T*>(this);
}

template <typename T>
inline void pool_item<T>::invalidate_ticket() noexcept
{
    _ticket->invalidate();
    _ticket = nullptr;
}


template <typename T>
void pool_item<T>::invalidate()
{
    _ticket->invalidate();
    _ticket = nullptr;
}
