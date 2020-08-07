#pragma once

#include "network/queues/queue.hpp"


// Basically this queue does nothing, whatever the packer does
// it just relays

template <typename Packer>
class eventually_synced_queue : public Packer
{
    friend class rpc::protocol_queues;
};
