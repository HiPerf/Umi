#pragma once
#include <kumo/opcodes.hpp>
#include <kumo/protocol_queues.hpp>
#include <kumo/structs.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/broadcaster.hpp>
namespace kumo
{
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_handshake_response(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, status&& data, T&& callback);
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_handshake_response(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, status&& data);
    template <typename B, typename T>
    void broadcast_handshake_response(::kaminari::broadcaster<B>* broadcaster, status&& data, T&& callback);
    template <typename B>
    void broadcast_handshake_response(::kaminari::broadcaster<B>* broadcaster, status&& data);
    template <typename B, typename T>
    void broadcast_handshake_response_single(::kaminari::broadcaster<B>* broadcaster, status&& data, T&& callback);
    template <typename B>
    void broadcast_handshake_response_single(::kaminari::broadcaster<B>* broadcaster, status&& data);
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_login_response(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, status_ex&& data, T&& callback);
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_login_response(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, status_ex&& data);
    template <typename B, typename T>
    void broadcast_login_response(::kaminari::broadcaster<B>* broadcaster, status_ex&& data, T&& callback);
    template <typename B>
    void broadcast_login_response(::kaminari::broadcaster<B>* broadcaster, status_ex&& data);
    template <typename B, typename T>
    void broadcast_login_response_single(::kaminari::broadcaster<B>* broadcaster, status_ex&& data, T&& callback);
    template <typename B>
    void broadcast_login_response_single(::kaminari::broadcaster<B>* broadcaster, status_ex&& data);
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_characters_list(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, characters&& data, T&& callback);
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_characters_list(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, characters&& data);
    template <typename B, typename T>
    void broadcast_characters_list(::kaminari::broadcaster<B>* broadcaster, characters&& data, T&& callback);
    template <typename B>
    void broadcast_characters_list(::kaminari::broadcaster<B>* broadcaster, characters&& data);
    template <typename B, typename T>
    void broadcast_characters_list_single(::kaminari::broadcaster<B>* broadcaster, characters&& data, T&& callback);
    template <typename B>
    void broadcast_characters_list_single(::kaminari::broadcaster<B>* broadcaster, characters&& data);
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_enter_world(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, success&& data, T&& callback);
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_enter_world(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, success&& data);
    template <typename B, typename T>
    void broadcast_enter_world(::kaminari::broadcaster<B>* broadcaster, success&& data, T&& callback);
    template <typename B>
    void broadcast_enter_world(::kaminari::broadcaster<B>* broadcaster, success&& data);
    template <typename B, typename T>
    void broadcast_enter_world_single(::kaminari::broadcaster<B>* broadcaster, success&& data, T&& callback);
    template <typename B>
    void broadcast_enter_world_single(::kaminari::broadcaster<B>* broadcaster, success&& data);
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_do_sth(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, complex&& data, T&& callback);
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_do_sth(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, complex&& data);
    template <typename B, typename T>
    void broadcast_do_sth(::kaminari::broadcaster<B>* broadcaster, complex&& data, T&& callback);
    template <typename B>
    void broadcast_do_sth(::kaminari::broadcaster<B>* broadcaster, complex&& data);
    template <typename B, typename T>
    void broadcast_do_sth_single(::kaminari::broadcaster<B>* broadcaster, complex&& data, T&& callback);
    template <typename B>
    void broadcast_do_sth_single(::kaminari::broadcaster<B>* broadcaster, complex&& data);
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_spawn(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, spawn_data&& data, T&& callback);
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_spawn(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, spawn_data&& data);
    template <typename B, typename T>
    void broadcast_spawn(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data, T&& callback);
    template <typename B>
    void broadcast_spawn(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data);
    template <typename B, typename T>
    void broadcast_spawn_single(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data, T&& callback);
    template <typename B>
    void broadcast_spawn_single(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data);
}

namespace kumo
{
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_handshake_response(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, status&& data, T&& callback)
    {
        pq->send_reliable(opcode::handshake_response, std::move(data), std::forward<T>(callback));
    }
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_handshake_response(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, status&& data)
    {
        pq->send_reliable(opcode::handshake_response, std::move(data));
    }
    template <typename B, typename T>
    void broadcast_handshake_response(::kaminari::broadcaster<B>* broadcaster, status&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::handshake_response, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_handshake_response(::kaminari::broadcaster<B>* broadcaster, status&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::handshake_response);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B, typename T>
    void broadcast_handshake_response_single(::kaminari::broadcaster<B>* broadcaster, status&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::handshake_response, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_handshake_response_single(::kaminari::broadcaster<B>* broadcaster, status&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::handshake_response);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_login_response(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, status_ex&& data, T&& callback)
    {
        pq->send_reliable(opcode::login_response, std::move(data), std::forward<T>(callback));
    }
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_login_response(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, status_ex&& data)
    {
        pq->send_reliable(opcode::login_response, std::move(data));
    }
    template <typename B, typename T>
    void broadcast_login_response(::kaminari::broadcaster<B>* broadcaster, status_ex&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::login_response, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_login_response(::kaminari::broadcaster<B>* broadcaster, status_ex&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::login_response);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B, typename T>
    void broadcast_login_response_single(::kaminari::broadcaster<B>* broadcaster, status_ex&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::login_response, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_login_response_single(::kaminari::broadcaster<B>* broadcaster, status_ex&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::login_response);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_characters_list(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, characters&& data, T&& callback)
    {
        pq->send_reliable(opcode::characters_list, std::move(data), std::forward<T>(callback));
    }
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_characters_list(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, characters&& data)
    {
        pq->send_reliable(opcode::characters_list, std::move(data));
    }
    template <typename B, typename T>
    void broadcast_characters_list(::kaminari::broadcaster<B>* broadcaster, characters&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::characters_list, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_characters_list(::kaminari::broadcaster<B>* broadcaster, characters&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::characters_list);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B, typename T>
    void broadcast_characters_list_single(::kaminari::broadcaster<B>* broadcaster, characters&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::characters_list, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_characters_list_single(::kaminari::broadcaster<B>* broadcaster, characters&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::characters_list);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_enter_world(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, success&& data, T&& callback)
    {
        pq->send_reliable(opcode::enter_world, std::move(data), std::forward<T>(callback));
    }
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_enter_world(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, success&& data)
    {
        pq->send_reliable(opcode::enter_world, std::move(data));
    }
    template <typename B, typename T>
    void broadcast_enter_world(::kaminari::broadcaster<B>* broadcaster, success&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::enter_world, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_enter_world(::kaminari::broadcaster<B>* broadcaster, success&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::enter_world);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B, typename T>
    void broadcast_enter_world_single(::kaminari::broadcaster<B>* broadcaster, success&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::enter_world, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_enter_world_single(::kaminari::broadcaster<B>* broadcaster, success&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::enter_world);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_do_sth(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, complex&& data, T&& callback)
    {
        pq->send_reliable(opcode::do_sth, std::move(data), std::forward<T>(callback));
    }
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_do_sth(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, complex&& data)
    {
        pq->send_reliable(opcode::do_sth, std::move(data));
    }
    template <typename B, typename T>
    void broadcast_do_sth(::kaminari::broadcaster<B>* broadcaster, complex&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::do_sth, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_do_sth(::kaminari::broadcaster<B>* broadcaster, complex&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::do_sth);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B, typename T>
    void broadcast_do_sth_single(::kaminari::broadcaster<B>* broadcaster, complex&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::do_sth, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_do_sth_single(::kaminari::broadcaster<B>* broadcaster, complex&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::do_sth);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <class ReliableAllocator, class OrderedAllocator, typename T>
    inline void send_spawn(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, spawn_data&& data, T&& callback)
    {
        pq->send_reliable(opcode::spawn, std::move(data), std::forward<T>(callback));
    }
    template <class ReliableAllocator, class OrderedAllocator>
    inline void send_spawn(::kumo::protocol_queues<ReliableAllocator, OrderedAllocator>* pq, spawn_data&& data)
    {
        pq->send_reliable(opcode::spawn, std::move(data));
    }
    template <typename B, typename T>
    void broadcast_spawn(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::spawn, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_spawn(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::spawn);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B, typename T>
    void broadcast_spawn_single(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data, T&& callback)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::spawn, std::forward<T>(callback));
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
    template <typename B>
    void broadcast_spawn_single(::kaminari::broadcaster<B>* broadcaster, spawn_data&& data)
    {
        boost::intrusive_ptr<::kaminari::packet> packet = ::kaminari::packet::make((uint16_t)opcode::spawn);
        ::kumo::marshal::pack(packet, data);
        broadcaster->broadcast([packet](auto pq) {
            pq->send_reliable(packet);
        });
    }
}
