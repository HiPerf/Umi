#pragma once

#include "containers/ticket.hpp"


template <typename D> class entity;

class unsafe_ticket_ref
{
public:
    template <typename T>
    static auto from(typename ticket<entity<T>>::ptr ticket)
    {
        return unsafe_ticket_ref(reinterpret_cast<uintptr_t*>(ticket.get()));
    }

    template <typename T>
    T* get() const
    {
        auto ticket = reinterpret_cast<::ticket<entity<T>>*>(_ticket);
        if (ticket->valid())
        {
            return ticket->get()->derived();
        }
        return nullptr;
    }

private:
    unsafe_ticket_ref(uintptr_t* ticket) :
        _ticket(ticket)
    {}

private:
    uintptr_t* _ticket;
};
