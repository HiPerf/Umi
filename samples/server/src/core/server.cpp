#include "core/server.hpp"

#include <kaminari/types/data_wrapper.hpp>
#include <kumo/rpc.hpp>

#include <pools/singleton_pool.hpp>
#include <fiber/exclusive_shared_work.hpp>
#include <fiber/yield.hpp>

#include <mutex>
#include <condition_variable>

#ifdef _MSC_VER 
    #include <timeapi.h>
    #pragma comment(lib, "Winmm.lib")
#endif // _MSC_VER 



server::server(uint16_t port, uint8_t num_server_workers, uint8_t num_network_workers) :
    base_executor<server>(),
    _store(),
    _map_scheme(_store),
    _client_scheme(_store),
    _transaction_scheme(_store),
    _last_tick(std_clock_t::now()),
    _diff_mean(static_cast<float>(HeartBeat.count())),
    _context(num_network_workers),
    _work(boost::asio::make_work_guard(_context)),
    _socket(_context, udp::endpoint(udp::v4(), port)),
    _database_async(2, 128),
    _stop(false)
{
    // Set instance
    instance = this;

    // Spawn executor threads
    base_executor<server>::start(num_server_workers, false);

    // Spawn network threads
    spawn_network_threads(num_network_workers);
}

void server::on_worker_thread()
{
    kaminari_packets_pool.this_thread_sinks();
}

void server::mainloop()
{
#ifdef _MSC_VER
    timeBeginPeriod(1);
#endif

    auto map_updater = _map_scheme.make_updater(false);
    auto client_updater = _client_scheme.make_updater(true);

    while (!_stop)
    {
        auto now = std_clock_t::now();
        auto diff = elapsed(_last_tick, now);
        _diff_mean = 0.95f * _diff_mean + 0.05f * diff.count();

        // Execute tasks (basically, new clients and data)
        base_executor<server>::execute_tasks();

        // Execute client inputs
        base_executor<server>::update(client_updater, update_inputs, std::ref(diff));

        // Update maps and execute tasks (which will be maps related)
        base_executor<server>::update(map_updater, std::ref(diff));
        base_executor<server>::sync(map_updater, std::ref(diff));
        base_executor<server>::execute_tasks();

        // Execute client outputs
        base_executor<server>::update(client_updater, update_outputs, std::ref(diff));

        // Rebalance pools
        kaminari_data_pool.rebalance();
        kaminari_packets_pool.rebalance();
        endpoints_pool.rebalance();

        // Sleep
        _last_tick = now;
        auto diff_mean = base_time(static_cast<uint64_t>(std::ceil(_diff_mean)));
        auto update_time = elapsed(now, std_clock_t::now()) + (diff_mean - HeartBeat);
        if (update_time < HeartBeat)
        {
            auto sleep_time = HeartBeat - update_time;
            std::cout << "Diff / Sleep / Mean = " << diff.count() << "/" << sleep_time.count() << "/" << _diff_mean << std::endl;

#ifdef _MSC_VER
            // SEE https://developercommunity.visualstudio.com/content/problem/58530/bogus-stdthis-threadsleep-for-implementation.html
            // AND https://github.com/microsoft/STL/issues/718
            Sleep(sleep_time.count());
#else
            std::this_thread::sleep_for(sleep_time);
#endif
        }
    }

#ifdef _MSC_VER
    timeEndPeriod(1);
#endif
}

void server::stop()
{
    _stop = true;
    base_executor<server>::stop();

    for (auto& t : _network_threads)
    {
        t.join();
    }

    _work.reset();
}

client* server::get_client(const udp::endpoint& endpoint) const
{
    if (auto it = _clients.find(endpoint); it != _clients.end())
    {
        if (auto client = it->second.get(); client->valid())
        {
            return client->get()->derived();
        }
    }
    
    return nullptr;
}

map* server::get_map(uint64_t id) const
{
    if (auto it = _maps.find(id); it != _maps.end())
    {
        return it->second;
    }
    
    return nullptr;
}

void server::create_client_transaction(uint64_t id)
{
    if (auto transaction = _transaction_scheme.get<class transaction>().get_derived_or_null(id))
    {
        transaction->unflag_deletion();
    }
    else
    {
        _transaction_scheme.alloc<class transaction>(id, static_cast<uint64_t>(base_time(std::chrono::seconds(5)).count()));
    }
}

void server::disconnect_client(client* client)
{
    client->flag_disconnecting();

    base_executor<server>::schedule([this, ticket=client->ticket()]() {
        if (auto client = ticket->get<class client>())
        {
            std::cout << "DISCONNECT AT " << client->endpoint() << std::endl;

            _clients.erase(client->endpoint());
            _client_scheme.free(client);
        }
    });
}

void server::send_client_outputs(client* client)
{
    std::cout << "SEND " << client->super_packet()->buffer().size() << "b TO " << client->endpoint() << std::endl;

    _socket.async_send_to(client->super_packet()->buffer(), client->endpoint(), [](const boost::system::error_code& error, std::size_t bytes) {
        if (error)
        {
            // TODO(gpascualg): Do something in case of error
        }
    });
}

void server::spawn_network_threads(uint8_t count)
{
    std::mutex m;
    std::condition_variable cv;
    uint8_t waiting = count;

    for (int i = 0; i < count; ++i)
    {
        _network_threads.push_back(std::thread([this, &waiting, &m, &cv] {
            // This is a sink for endpoints and data
            endpoints_pool.this_thread_sinks();
            kaminari_data_pool.this_thread_sinks();

            std::unique_lock<std::mutex> lk(m);
            --waiting;
            cv.notify_one();
            lk.unlock();

            _context.run();
        }));

        handle_connections();
    }

    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [&] { return waiting == 0; });
}

void server::handle_connections()
{
    // Get a new buffer
    ::kaminari::data_wrapper* buffer = kaminari_data_pool.get();
    udp::endpoint* accept_endpoint = endpoints_pool.get();

    _socket.async_receive_from(boost::asio::buffer(buffer->data, 500), *accept_endpoint, 0, [this, buffer, accept_endpoint](const auto& error, std::size_t bytes) {
        std::cout << "Incoming packet from " << *accept_endpoint << " [" << bytes << "b, " << static_cast<bool>(error) << "]" << std::endl;

        if (error)
        {
            kaminari_data_pool.release(buffer);
        }
        else
        {
            // Set read size
            buffer->size = bytes;

            // Create client
            bool operation_allowed = server::instance->get_or_create_client(accept_endpoint, [this, accept_endpoint, buffer](auto client) {
                // Release buffer
                endpoints_pool.release(accept_endpoint);

                // Add packet
                client->received_packet(boost::intrusive_ptr<::kaminari::data_wrapper>(buffer));
            });

            // Free buffer it it failed
            if (!operation_allowed)
            {
                kaminari_data_pool.release(buffer);
                endpoints_pool.release(accept_endpoint);
            }
        }

        // Handle again
        handle_connections();
    });
}

void release_data_wrapper(::kaminari::data_wrapper* x)
{
    server::instance->kaminari_data_pool.release(x);
}

::kaminari::packet* get_kaminari_packet(uint16_t opcode)
{
    return server::instance->kaminari_packets_pool.get(opcode);
}

void release_kaminari_packet(::kaminari::packet* packet)
{
    server::instance->kaminari_packets_pool.release(packet);
}
