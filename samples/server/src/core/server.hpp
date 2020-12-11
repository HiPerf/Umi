#pragma once

#include <boost/asio.hpp>

#include "core/client.hpp"
#include "maps/map.hpp"

#include <async/async_executor.hpp>
#include <database/transaction.hpp>
#include <entity/scheme.hpp>
#include <updater/executor.hpp>
#include <pools/thread_local_pool.hpp>

// TODO(gpascualg): This is a test used on client creation, remove me
#include <kumo/rpc.hpp>


#include <unordered_map>
#include <unordered_set>
#include <chrono>


namespace std
{
    template <>
    struct hash<udp::endpoint>
    {
        size_t operator()(udp::endpoint const& v) const {
            uint64_t seed = 0;
            boost::hash_combine(seed, v.address().to_v4().to_uint());
            boost::hash_combine(seed, v.port());
            return seed;
        }
    };
}



constexpr uint32_t prereserved_size = 256;

class server : protected base_executor<server>
{
    friend class base_executor<server>;
    template <typename T, uint32_t S = prereserved_size> using dic = dictionary<T, entity<T>, S>;

public:
    static inline server* instance = nullptr;

public:
    server(uint16_t port, uint8_t num_server_workers, uint8_t num_network_workers);

    void on_worker_thread();
    void mainloop();
    void stop();

    // UNSAFE methods, can only if:
    //  a) Absolute certainty the object exists
    //  b) Inside the main thread
    client* get_client(const udp::endpoint& endpoint) const;
    map* get_map(uint64_t id) const;

    transaction* get_entity_transaction(uint64_t id) const;
    void create_entity_transaction(uint64_t id);
    void schedule_entity_transaction_removal(transaction* t);

    // SAFE methods, to be used inside any updater
    template <typename C>
    bool get_or_create_client(udp::endpoint* endpoint, C&& callback);

    template <typename C>
    void get_or_create_map(uint64_t id, C&& callback);
    
    void disconnect_client(client* client);
    void send_client_outputs(client* client);

    inline async_executor<(uint16_t)FiberID::DatabaseWorker>& database_async();
    
    template <typename F>
    void schedule(F&& functions);

    template <typename T, typename F>
    void schedule_if(T&& ticket, F&& functions);

private:
    void spawn_network_threads(uint8_t count);
    void handle_connections();

private:
    // Data schemes
    scheme_store<dic<map>, dic<client>, dic<transaction>> _store;
    decltype(scheme_maker<map>()(_store)) _map_scheme;
    decltype(scheme_maker<client>()(_store)) _client_scheme;
    decltype(scheme_maker<transaction>()(_store)) _transaction_scheme;

    // Clients
    std::unordered_map<udp::endpoint, ticket<entity<client>>::ptr> _clients;
    std::unordered_set<udp::endpoint> _blacklist;

    // Maps
    std::unordered_map<uint64_t, map*> _maps;

    // Constant timestep
    std_clock_t::time_point _last_tick;
    float _diff_mean;

    // Networking
    std::vector<std::thread> _network_threads;
    boost::asio::io_context _context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _work;
    udp::socket _socket;

    // Database
    async_executor<(uint16_t)FiberID::DatabaseWorker> _database_async;

    bool _stop;

    // For convinience, all pools are public
public:
    thread_local_pool<::kaminari::data_wrapper, 255> kaminari_data_pool;
    thread_local_pool<::kaminari::packet, 255> kaminari_packets_pool;
    thread_local_pool<udp::endpoint, 255> endpoints_pool;
};


template <typename C>
bool server::get_or_create_client(udp::endpoint* endpoint, C&& callback)
{
    // TODO(gpascualg): This is not thread safe
    if (auto it = _blacklist.find(*endpoint); it != _blacklist.end())
    {
        return false;
    }

    base_executor<server>::create_with_pointer_precondition<class client>(
        _client_scheme, 
        // Precondition, client might already exist
        [this, endpoint]() {
            return get_client(*endpoint);
        },
        // If created, emplace it in the map
        [this, endpoint](auto client) {
            _clients.emplace(*endpoint, client->ticket());
            std::cout << "NEW CLIENT AT " << client->endpoint() << " (" << *endpoint << ")" << std::endl;
            
            // TODO(gpascualg): This is a test, remove me
            kumo::send_spawn(client->super_packet(), { .id = 55, .x = 1, .y = 5 });

            return tao::tuple(client);
        },
        // Standard callback
        std::move(callback),
        _client_scheme.args<client>(std::cref(*endpoint))
    );

    return true;
} 

template <typename C>
void server::get_or_create_map(uint64_t id, C&& callback)
{
    base_executor<server>::create_with_pointer_precondition<class map>(
        _map_scheme, 
        // Precondition, map might already exist
        [this, id]() {
            return get_map(id);
        },
        // If created, emplace it in the map
        [this, id](auto map) {
            _maps.emplace(id, map);
            return tao::tuple(map);
        },
        // Standard callback
        std::move(callback),
        _map_scheme.args<map>());
} 

inline async_executor<(uint16_t)FiberID::DatabaseWorker>& server::database_async()
{
    return _database_async;
}

template <typename F>
void server::schedule(F&& function)
{
    base_executor<server>::schedule(std::move(function));
}

template <typename T, typename F>
void server::schedule_if(T&& ticket, F&& function)
{
    base_executor<server>::schedule([ticket, function{ std::move(function) }]()
        {
            if (ticket->valid())
            {
                function(ticket->get()->derived());
            }
        });
}
