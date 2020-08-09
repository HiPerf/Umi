#pragma once
#include <kumo/opcodes.hpp>
#include <kumo/protocol_queues.hpp>
#include <kumo/structs.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/broadcaster.hpp>
namespace kumo
{
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_do_sth(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, complex&& data, T&& callback);
    template <typename B, typename T>
    void broadcast_do_sth(::kaminari::broadcaster<B>* broadcaster, complex&& data, T&& callback);
    template <typename B, typename T>
    void broadcast_do_sth_single(::kaminari::broadcaster<B>* broadcaster, complex&& data, T&& callback);
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_spawn(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, spawn_data&& data, T&& callback);
    template <typename B, typename T>
    void broadcast_spawn(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data, T&& callback);
    template <typename B, typename T>
    void broadcast_spawn_single(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data, T&& callback);
}

namespace kumo
{
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_do_sth(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, complex&& data, T&& callback)
    {
        pq->send_reliable(static_cast<uint16_t>(opcode::do_sth), std::move(data), std::forward<T>(callback));
    }
    template <typename B, typename T>
    void broadcast_do_sth(::kaminari::broadcaster<B>* broadcaster, complex&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make(static_cast<uint16_t>(opcode::do_sth), std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B, typename T>
    void broadcast_do_sth_single(::kaminari::broadcaster<B>* broadcaster, complex&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make(static_cast<uint16_t>(opcode::do_sth), std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_spawn(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, spawn_data&& data, T&& callback)
    {
        pq->send_reliable(static_cast<uint16_t>(opcode::spawn), std::move(data), std::forward<T>(callback));
    }
    template <typename B, typename T>
    void broadcast_spawn(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make(static_cast<uint16_t>(opcode::spawn), std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B, typename T>
    void broadcast_spawn_single(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make(static_cast<uint16_t>(opcode::spawn), std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
}