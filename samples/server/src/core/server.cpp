#include "core/server.hpp"

#include <kaminari/types/data_wrapper.hpp>

#include <containers/plain_pool.hpp>
#include <fiber/exclusive_shared_work.hpp>
#include <fiber/yield.hpp>


#ifdef _MSC_VER 
    #include <timeapi.h>
    #pragma comment(lib, "Winmm.lib")
#endif // _MSC_VER 



server::server(uint16_t port, uint8_t num_server_workers, uint8_t num_network_workers) :
    executor(num_server_workers, false),
    _store(),
    _map_scheme(_map_scheme.make(_store)),
    _client_scheme(_client_scheme.make(_store)),
    _last_tick(std_clock_t::now()),
    _diff_mean(static_cast<float>(HeartBeat.count())),
    _context(num_network_workers),
    _work(boost::asio::make_work_guard(_context)),
    _socket(_context, udp::endpoint(udp::v4(), port)),
    _stop(false)
{
    instance = this;

    // Create pools
    singleton_pool<::kaminari::data_wrapper>::make(sizeof(::kaminari::data_wrapper));
    singleton_pool<::kaminari::packet>::make(sizeof(::kaminari::packet));

    // Spawn network threads
    spawn_network_threads(num_network_workers);
}


void server::mainloop()
{
#ifdef _MSC_VER
    timeBeginPeriod(1);
#endif

    auto map_updater = _map_scheme.make_updater(false);
    auto client_updater = _map_scheme.make_updater(true);

    get_or_create_client(udp::endpoint(), [](auto client) {
        return std::tuple(client);
    });

    while (!_stop)
    {
        auto now = std_clock_t::now();
        auto diff = elapsed(_last_tick, now);
        _diff_mean = 0.95f * _diff_mean + 0.05f * diff.count();

        // Execute client inputs
        executor::update(client_updater, update_inputs, std::ref(diff));

        // Update maps and execute tasks
        executor::update(map_updater, std::ref(diff));
        executor::sync(map_updater, std::ref(diff));
        executor::execute_tasks();

        // Execute client outputs
        executor::update(client_updater, update_outputs, std::ref(diff));

        _last_tick = now;
        auto update_time = elapsed(now, std_clock_t::now());
        if (update_time < HeartBeat)
        {
            auto diff_mean = base_time(static_cast<uint64_t>(std::ceil(_diff_mean)));
            auto sleep_time = HeartBeat - update_time - (diff_mean - HeartBeat);
            std::cout << "Diff / Sleep / Mean = " << diff.count() << "/" << sleep_time.count() << "/" << _diff_mean << std::endl;
            std::this_thread::sleep_for(sleep_time);
        }
    }

#ifdef _MSC_VER
    timeEndPeriod(1);
#endif
}

void server::stop()
{
    _stop = true;
    executor::stop();
}

client* server::get_client(const udp::endpoint& endpoint) const
{
    if (auto it = _clients.find(endpoint); it != _clients.end())
    {
        if (auto client = it->second.get())
        {
            return client->derived();
        }
    }
    
    return nullptr;
}

void server::send_client_outputs(client* client)
{
    _socket.async_send_to(client->super_packet()->buffer(), client->endpoint(), [](const boost::system::error_code& error, std::size_t bytes) {
        if (error)
        {
            // TODO(gpascualg): Do something in case of error
        }
    });
}

void server::spawn_network_threads(uint8_t count)
{
    boost::fibers::barrier network_barrier(count + 1);
    boost::fibers::barrier network_algo_barrier(count + 1);

    for (int i = 0; i < count; ++i)
    {
        _network_threads.push_back(std::thread([this, &network_barrier, &network_algo_barrier, count] {
            network_barrier.wait();

            boost::fibers::use_scheduling_algorithm<boost::fibers::algo::exclusive_shared_work<int(FiberID::NetworkWorker)>>(count);
            network_algo_barrier.wait();

            boost::fibers::fiber([this]() { handle_connections(); }).join();
        }));
    }

    network_barrier.wait();
    network_algo_barrier.wait();
}

void server::handle_connections()
{
    while (!_stop)
    {
        // Get a new client
        ::kaminari::data_wrapper* buffer = singleton_pool<::kaminari::data_wrapper>::instance->get();
        udp::endpoint acceptEndpoint;
        boost::system::error_code error;

        buffer->size = static_cast<uint16_t>(_socket.async_receive_from(boost::asio::buffer(buffer->data, 500),
            acceptEndpoint, 0, boost::fibers::asio::yield[error]));

        if (!error)
        {
            bool client_creation = server::instance->get_or_create_client(acceptEndpoint, [buffer](auto client) {
                client->received_packet(boost::intrusive_ptr<::kaminari::data_wrapper>(buffer));
                return std::tuple(client);
            });

            if (!client_creation)
            {
                singleton_pool<::kaminari::data_wrapper>::instance->free(buffer);
            }
        }
        else
        {
            singleton_pool<::kaminari::data_wrapper>::instance->free(buffer);
        }
    }
}

void release_data_wrapper(::kaminari::data_wrapper* x)
{
    singleton_pool<::kaminari::data_wrapper>::instance->free(x);
}

::kaminari::packet* get_kaminari_packet(uint16_t opcode)
{
    return singleton_pool<::kaminari::packet>::instance->get(opcode);
}

void release_kaminari_packet(::kaminari::packet* packet)
{
    singleton_pool<::kaminari::packet>::instance->free(packet);
}
