#pragma once

#include "containers/concepts.hpp"
#include "containers/ticket.hpp"



template <typename T>
class pool_item
{
    template <typename D, typename E, uint16_t I, typename R> friend class pooled_static_vector;

public:
    pool_item();
    pool_item(const pool_item&) = delete;
    pool_item(pool_item&& other);
    pool_item& operator=(const pool_item&) = delete;
    pool_item& operator=(pool_item&& other);

    inline typename ticket<T>::ptr ticket() { return _ticket; }

protected:
    void invalidate();

private:
   typename ::ticket<T>::ptr _ticket;
};

template <typename T>
pool_item<T>::pool_item() :
    _ticket(nullptr)
{}

template <typename T>
pool_item<T>::pool_item(pool_item&& other) :
    _ticket(std::move(other._ticket))
{
    _ticket->_ptr = reinterpret_cast<T*>(this);
}

template <typename T>
pool_item<T>& pool_item<T>::operator=(pool_item&& other)
{
    _ticket = std::move(other._ticket);
    _ticket->_ptr = reinterpret_cast<T*>(this);
    return *this;
}

template <typename T>
void pool_item<T>::invalidate()
{
    _ticket->invalidate();
    _ticket = nullptr;
}
