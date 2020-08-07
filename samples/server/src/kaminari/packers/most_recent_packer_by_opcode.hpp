#pragma once

#include "common/types.hpp"
#include "network/packers/packer.hpp"
#include "network/autogen/marshal.hpp"

#include <unordered_map>


struct packet_by_opcode
{
    packet_by_opcode(Packet::Ptr p, rpc::Opcode o) :
        packet(p),
        opcode(o)
    {}

    Packet::Ptr packet;
    rpc::Opcode opcode;
};


template <class Allocator = std::allocator<detail::pending_data<packet_by_opcode>>>
class most_recent_packer_by_opcode : protected packer<most_recent_packer_by_opcode, packet_by_opcode, Allocator>
{
    template <class A> friend class packer<most_recent_packer_by_opcode<A>, packet_by_opcode, A>;

public:
    using packer_t = packer<most_recent_packer_by_opcode, packet_by_opcode, Allocator>;

public:
    using packer<most_recent_packer_by_opcode, packet_by_opcode, Allocator>::packer;
    
    template <typename T, typename... Args>
    void add(rpc::Opcode opcode, T&& data, Args&&... args);
    void process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block);

private:
    void add(const Packet::Ptr& packet, rpc::Opcode opcode);

protected:
    inline void on_ack(const pending_vector_t::iterator& part);
    inline void clear();

protected:
    std::unordered_map<rpc::Opcode, typename packer_t::pending_data*> _opcode_map;
};


template <class Allocator>
template <typename T, typename... Args>
void most_recent_packer_by_opcode<Allocator>::add(rpc::Opcode opcode, T&& data, Args&&... args)
{
    // Immediate mode means that the structure is packed right now
    Packet::Ptr packet = Packet::make(opcode, std::forward<Args>(args)...);
    rpc::marshal::pack_message<T>(packet, data);

    // Add to pending
    add(packet, opcode);
}

template <class Allocator>
void most_recent_packer_by_opcode<Allocator>::add(const Packet::Ptr& packet, rpc::Opcode opcode)
{
    // Add to pending
    if (auto it = _opcode_map.find(opcode); it != _opcode_map.end())
    {
        auto pending = it->second;
        pending->data.packet = packet;
        pending->blocks.clear();
    }
    else
    {
        // Add to pending
        auto ptr = _allocator.allocate(1);
        auto pending = new (ptr) detail::pending_data<packet_by_opcode>(packet, opcode);
        packer_t::_pending.push_back(pending);
        _opcode_map.emplace(opcode, pending);
    }
}

template <class Allocator>
void most_recent_packer_by_opcode<Allocator>::process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block)
{
    for (auto& pending : _pending)
    {
        if (!is_pending(pending->blocks, block_id, false))
        {
            continue;
        }

        uint16_t actual_block = get_actual_block(pending->blocks, block_id);
        uint16_t size = pending->data.packet->size();
        if (auto it = by_block.find(actual_block); it != by_block.end())
        {
            // TODO(gpascualg): Do we want a hard-break here? packets in the vector should probably be
            //  ordered by size? But we could starve big packets that way, it requires some "agitation"
            //  factor for packets being ignored too much time
            if (size > remaining)
            {
                break;
            }

            it->second.push_back(pending->data.packet);
        }
        else
        {
            // TODO(gpascualg): Magic numbers, 4 is block header + block size
            // TODO(gpascualg): This can be brought down to 3, block header + packet count
            size += 4;

            // TODO(gpascualg): Same as above, do we want to hard-break?
            if (size > remaining)
            {
                break;
            }

            by_block.emplace(actual_block, std::initializer_list<Packet::Ptr> { pending->data.packet });
        }

        pending->blocks.push_back(block_id);
        remaining -= size;
    }
}

template <class Allocator>
inline void most_recent_packer_by_opcode<Allocator>::on_ack(const pending_vector_t::iterator& part)
{
    // Erased acked entities
    for (auto it = part; it != packer_t::_pending.end(); ++it)
    {
        _opcode_map.erase((*it)->data.opcode);
    }
}

template <class Allocator>
inline void most_recent_packer_by_opcode<Allocator>::clear()
{
    _opcode_map.clear();
}
