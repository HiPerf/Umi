#pragma once

#include "network/packers/packer.hpp"
#include "network/autogen/marshal.hpp"


template <typename Global, typename Detail, uint16_t opcode, class Allocator = std::allocator<detail::pending_data<Detail>>>
class merge_packer : protected packer<merge_packer<Global, Detail, opcode, Allocator>, Detail, Allocator>
{
    template <class A> friend class packer<merge_packer<Global, Detail, opcode, A>, Detail, A>;

public:
    using packer_t = packer<merge_packer<Global, Detail, opcode, Allocator>, Detail, Allocator>;
    using pending_vector_t = typename packer_t::pending_vector_t;

public:
    using packer<merge_packer<Global, Detail, opcode, Allocator>, Detail, Allocator>::packer;
    
    template <typename... Args>
    void add(uint16_t _unused, Detail&& data, Args&&... args);
    void process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block);

protected:
    inline void on_ack(const typename pending_vector_t::iterator& part);
    inline void clear();
};


template <typename Global, typename Detail, uint16_t opcode, class Allocator>
template <typename... Args>
void merge_packer<Global, Detail, opcode, Allocator>::add(uint16_t _unused, Detail&& data, Args&&... args)
{
    // Opcode is ignored
    (void)_unused;

    // In merge mode we add the detailed structure
    auto ptr = packer_t::_allocator.allocate(1);
    auto pending = new (ptr) detail::pending_data<Detail>(data);
    packer_t::_pending.push_back(pending);
}

template <typename Global, typename Detail, uint16_t opcode, class Allocator>
inline void merge_packer<Global, Detail, opcode, Allocator>::process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block)
{
    // Do not do useless jobs
    if (packer_t::_pending.empty())
    {
        return;
    }
    
    // Create the global structure
    Global global;

    // TODO(gpascualg): MAGIC NUMBERS, 2 is vector size
    uint16_t size = Packet::DataStart + 2 + packer_t::new_block_cost(block_id, by_block);

    // Populate it as big as we can
    for (auto& pending : packer_t::_pending)
    {
        if (!packer_t::is_pending(pending->blocks, block_id, false))
        {
            continue;
        }

        // If this one won't fit, neither will the rest
        auto next_size = size + rpc::marshal::message_size<Detail>(pending->data);
        if (next_size > remaining)
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
    rpc::marshal::pack_message<Global>(packet, global);
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

template <typename Global, typename Detail, uint16_t opcode, class Allocator>
inline void merge_packer<Global, Detail, opcode, Allocator>::on_ack(const typename pending_vector_t::iterator& part)
{
    // Nothing to do here
    (void)part;
}

template <typename Global, typename Detail, uint16_t opcode, class Allocator>
inline void merge_packer<Global, Detail, opcode, Allocator>::clear()
{}
