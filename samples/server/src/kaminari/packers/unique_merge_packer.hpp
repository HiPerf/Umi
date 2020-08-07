#pragma once

#include "common/types.hpp"
#include "network/packers/packer.hpp"
#include "network/autogen/marshal.hpp"

#include <unordered_map>


template <typename Global, typename Detail, uint16_t opcode, class Allocator = std::allocator<detail::pending_data<Detail>>>
class unique_merge_packer : protected packer<unique_merge_packer<Global, Detail, opcode, Allocator>, Detail, Allocator>
{
    template <class A> friend class packer<unique_merge_packer<Global, Detail, opcode, A>, Detail, A>;

public:
    using packer_t = packer<unique_merge_packer<Global, Detail, opcode, Allocator>, Detail, Allocator>;
    using pending_vector_t = typename packer_t::pending_vector_t;

public:
    using packer<unique_merge_packer<Global, Detail, opcode, Allocator>, Detail, Allocator>::packer;

    template <typename T, typename... Args>
    void add(uint16_t _unused, T&& data, Args&&... args);
    void process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block);

protected:
    inline void on_ack(const typename pending_vector_t::iterator& part);
    inline void clear();

protected:
    std::unordered_map<entity_id_t, typename packer_t::pending_data*> _id_map;
};


template <typename Global, typename Detail, uint16_t opcode, class Allocator>
template <typename T, typename... Args>
void unique_merge_packer<Global, Detail, opcode, Allocator>::add(uint16_t _unused, T&& data, Args&&... args)
{
    // Opcode is ignored
    (void)_unused;

    // Do we have this entity already?
    auto id = data.id;
    if (auto it = _id_map.find(id); it != _id_map.end())
    {
        auto pending = it->second;
        pending->data = data;
        pending->blocks.clear();
    }
    else
    {
        auto ptr = packer_t::_allocator.allocate(1);
        auto pending = new (ptr) detail::pending_data<Detail>(data);
        packer_t::_pending.push_back(pending);
        _id_map.emplace(id, pending);
    }
}

template <typename Global, typename Detail, uint16_t opcode, class Allocator>
inline void unique_merge_packer<Global, Detail, opcode, Allocator>::process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block)
{
    // Do not do useless jobs
    if (packer_t::_pending.empty())
    {
        return;
    }

    // We can not naively pack everything into a single packet, as it might
    // go beyond its maximum size
    bool outgrows_superpacket = false;
    auto it = packer_t::_pending.begin();
    while (!outgrows_superpacket && it != packer_t::_pending.end())
    {
        // Create the global structure
        Global global;

        // TODO(gpascualg): MAGIC NUMBERS, 2 is vector size
        uint16_t size = Packet::DataStart + 2 + packer_t::new_block_cost(block_id, by_block);

        // Populate it as big as we can
        for (; it != packer_t::_pending.end(); ++it)
        {
            auto& pending = *it;
            if (!packer_t::is_pending(pending->blocks, block_id, false))
            {
                continue;
            }

            // If this one won't fit, neither will the rest
            auto next_size = size + rpc::marshal::message_size<Detail>(pending->data);
            if (next_size > remaining)
            {
                outgrows_superpacket = true;
                break;
            }
            
            if (next_size > MAX_PACKET_SIZE)
            {
                break;
            }

            size = next_size;
            global.data.push_back(pending->data);
            pending->blocks.push_back(block_id);
        }

        // Nothing to do here
        if (global.data.empty())
        {
            return;
        }

        Packet::Ptr packet = Packet::make(opcode);
        rpc::marshal::pack_message(packet, global);
        remaining -= size;

        if (auto it = by_block.find(block_id); it != by_block.end())
        {
            it->second.push_back(packet);
        }
        else
        {
            by_block.emplace(block_id, std::initializer_list<Packet::Ptr> { packet });
        }
    }
}

template <typename Global, typename Detail, uint16_t opcode, class Allocator>
inline void unique_merge_packer<Global, Detail, opcode, Allocator>::on_ack(const typename pending_vector_t::iterator& part)
{
    // Erased acked entities
    for (auto it = part; it != packer_t::_pending.end(); ++it)
    {
        _id_map.erase((*it)->data.id);
    }
}

template <typename Global, typename Detail, uint16_t opcode, class Allocator>
inline void unique_merge_packer<Global, Detail, opcode, Allocator>::clear()
{
    _id_map.clear();
}
