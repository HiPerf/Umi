#pragma once

#include "common/tao.hpp"
#include "ids/generator.hpp"
#include "traits/shared_function.hpp"
#include "updater/tasks_manager.hpp"
#include "updater/updater.hpp"

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/barrier.hpp>

#include <tao/tuple/tuple.hpp>

#include <iostream>
#include <list>



template <typename D>
class base_executor;

template <typename D>
concept has_on_worker_thread = requires() {
    { std::declval<D>().on_worker_thread() };
};

template <typename D>
class base_executor : public tasks_manager
{
protected:
    static inline base_executor<D>* _instance = nullptr;

public:
    base_executor() noexcept :
        tasks_manager(),
        _stop(false)
    {
        _instance = this;
    }

    static inline base_executor<D>* instance()
    {
        return _instance;
    }

    inline bool stopped() const noexcept
    {
        return _stop;
    }

    void start(uint8_t num_workers, bool suspend) noexcept
    {
        for (uint8_t thread_id = 1; thread_id < num_workers; ++thread_id)
        {
            _workers.push_back(std::thread(
                [this, num_workers, suspend] {
                    // Custom behaviour, if any
                    if constexpr (!std::is_same_v<D, void> && has_on_worker_thread<D>)
                    {
                        static_cast<D&>(*this).on_worker_thread();
                    }

                    // Set thread algo
                    public_scheduling_algorithm<exclusive_work_stealing<0>>(num_workers, suspend);

                    _mutex.lock();
                    // suspend main-fiber from the worker thread
                    _cv.wait(_mutex, [this]() { return _stop; });
                    _mutex.unlock();
                })
            );
        }

        // Set thread algo
        public_scheduling_algorithm<exclusive_work_stealing<0>>(num_workers, suspend);
    }

    void stop() noexcept
    {
        // Notify, wait and join workers
        _stop = true;
        _cv.notify_all();

        for (auto& t : _workers)
        {
            t.join();
        }
    }

    template <typename U, typename... Args>
    constexpr void update(U& updater, Args&&... args) noexcept
    {
        boost::fibers::fiber([&updater, ...args{ std::forward<Args>(args) }]() mutable {
            updater.update(std::forward<Args>(args)...);
            updater.wait_update();
        }).join();
    }

    template <typename U, typename... Args>
    constexpr void update_no_wait(U& updater, Args&&... args) noexcept
    {
        boost::fibers::fiber([&updater, ...args{ std::forward<Args>(args) }]() mutable {
            updater.update(std::forward<Args>(args)...);
        }).join();
    }

    template <typename U, typename... Args>
    constexpr void wait_update(U& updater, Args&&... args) noexcept
    {
        boost::fibers::fiber([&updater, ...args{ std::forward<Args>(args) }]() mutable {
            updater.wait_update();
        }).join();
    }

    template <typename... U, typename... Args>
    constexpr void update_many(tao::tuple<Args...>&& args, U&... updaters) noexcept
    {
        boost::fibers::fiber([... updaters{ &updaters }, args{ std::forward<tao::tuple<Args...>>(args) }]() mutable {
            tao::apply([&](auto&&... args) {
                ((updaters->update(std::forward<decltype(args)>(args)...)), ...);
            }, args);
            
            (updaters->wait_update(), ...);
        }).join();
    }

    template <typename U, typename... Args>
    constexpr void sync(U& updater, Args&&... args) noexcept
    {
        updater.sync(std::forward<Args>(args)...);
    }

    template <template <typename...> typename S, typename... A, typename... vecs>
    constexpr uint64_t create(S<vecs...>& scheme, A&&... scheme_args) noexcept
    {
        return create_with_callback(scheme, [](auto&&... e) { return tao::tuple(e...); }, std::forward<A>(scheme_args)...);
    }

    template <template <typename...> typename S,typename... Args, typename... vecs>
    constexpr uint64_t create_with_args(S<vecs...>& scheme, Args&&... args) noexcept
    {
        return create_with_callback(scheme, [](auto&&... e) { return tao::tuple(e...); }, 
            scheme.template args<vecs>(std::forward<Args>(args)...)...
        );
    }

    template <template <typename...> typename S, typename C, typename... A, typename... vecs>
    constexpr uint64_t create_with_callback(S<vecs...>& scheme, C&& callback, A&&... scheme_args) noexcept
    {
        uint64_t id = id_generator().next();
        return create_with_callback(id, scheme, std::move(callback), std::forward<A>(scheme_args)...);
    }

    template <template <typename...> typename S, typename C, typename... A, typename... vecs>
    constexpr uint64_t create_with_callback_and_partition(bool p, S<vecs...>& scheme, C&& callback, A&&... scheme_args) noexcept
    {
        uint64_t id = id_generator().next();
        return create_with_callback_and_partition(p, id, scheme, std::move(callback), std::forward<A>(scheme_args)...);
    }

    //template <template <typename...> typename S, typename C, typename... A, typename... vecs>
    //__declspec(noinline) constexpr uint64_t create_with_callback(uint64_t id, S<vecs...>& scheme, C&& callback, A&&... scheme_args)
    //{
    //    static_assert(sizeof...(vecs) == sizeof...(scheme_args), "Incomplete scheme creation");

    //    schedule([id, callback{ std::move(callback) }, ...scheme_args{ std::move(scheme_args) }, &scheme] () {
    //        auto entities = callback(tao::apply([&](auto&&... args) {
    //            auto component = scheme_args.comp.alloc(id, std::forward<decay_t<decltype(args)>>(args)...);
    //            component->base()->scheme_information(scheme);
    //            return component;
    //        }, scheme_args.args)...);

    //        tao::apply([](auto&&... entities) {
    //            (..., entities->base()->scheme_created());
    //        }, std::move(entities));
    //    });

    //    return id;
    //}

    template <template <typename...> typename S, typename C, typename... A, typename... vecs>
    constexpr uint64_t create_with_callback(uint64_t id, S<vecs...>& scheme, C&& callback, A&&... scheme_args) noexcept
        requires (... && !std::is_lvalue_reference<A>::value)
    {
        static_assert(sizeof...(vecs) == sizeof...(scheme_args), "Incomplete scheme creation");

        schedule([
            this,
            id,
            &scheme,
            callback = std::move(callback),
            ... scheme_args = std::forward<A>(scheme_args)
        ] () mutable {
            // Create entities by using each allocator and arguments
            // Call callback now too
            auto entities = tao::tuple(create(id, scheme, std::move(scheme_args)) ...);

            // Create dynamic content
            auto map = std::make_shared<components_map>(entities);

            // Notify of complete scheme creation
            tao::apply([&map](auto&&... entities) mutable {
                (..., entities->base()->scheme_created(map));
            }, entities);

            tao::apply(std::move(callback), std::move(entities));
        });

        return id;
    }

    template <template <typename...> typename S, typename... A, typename... vecs>
    auto create_unsafe(uint64_t id, S<vecs...>& scheme, A&&... scheme_args) noexcept
        requires (... && !std::is_lvalue_reference<A>::value)
    {
        static_assert(sizeof...(vecs) == sizeof...(scheme_args), "Incomplete scheme creation");

        auto entities = tao::tuple(create(id, scheme, std::move(scheme_args)) ...);

        // Create dynamic content
        auto map = std::make_shared<components_map>(entities);

        // Notify of complete scheme creation
        tao::apply([&map](auto&&... entities) mutable {
            (..., entities->base()->scheme_created(map));
        }, entities);

        return entities;
    }

    template <template <typename...> typename S, typename C, typename... A, typename... vecs>
    constexpr uint64_t create_with_callback_and_partition(bool p, uint64_t id, S<vecs...>& scheme, C&& callback, A&&... scheme_args) noexcept
        requires (... && !std::is_lvalue_reference<A>::value)
    {
        static_assert(sizeof...(vecs) == sizeof...(scheme_args), "Incomplete scheme creation");

        schedule([
            this,
            p,
            id,
            &scheme,
            callback = std::move(callback),
            ... scheme_args = std::forward<A>(scheme_args)
        ] () mutable {
            // Create entities by using each allocator and arguments
            // Call callback now too
            auto entities = tao::tuple(create_with_partition(p, id, scheme, std::move(scheme_args)) ...);

            // Create dynamic content
            auto map = std::make_shared<components_map>(entities);

            // Notify of complete scheme creation
            tao::apply([&map](auto&&... entities) mutable {
                (..., entities->base()->scheme_created(map));
            }, entities);

            tao::apply(std::move(callback), std::move(entities));
        });

        return id;
    }

    template <template <typename...> typename S, typename PC, typename CC, typename AC, typename... A, typename... vecs>
    constexpr void create_with_precondition(S<vecs...>& scheme, PC&& precondition, CC&& created_callback, AC&& always_callback, A&&... scheme_args) noexcept
        requires (... && !std::is_lvalue_reference<A>::value)
    {
        static_assert(sizeof...(vecs) == sizeof...(scheme_args), "Incomplete scheme creation");

        schedule([
            this,
            &scheme,
            precondition = std::move(precondition),
            created_callback = std::move(created_callback),
            always_callback = std::move(always_callback),
            ... scheme_args = std::forward<A>(scheme_args)
        ] () mutable {
            auto optional_tuple = precondition();
            if (optional_tuple)
            {
                tao::apply(std::move(always_callback), std::move(*optional_tuple));
                return;
            }
            
            uint64_t id = id_generator().next();
            // Create entities by using each allocator and arguments
            // Call callback now too
            auto entities = tao::apply(std::move(created_callback), tao::forward_as_tuple(create(id, scheme, std::move(scheme_args)) ...));

            // Create dynamic content
            auto map = std::make_shared<components_map>(entities);

            // Notify of complete scheme creation
            tao::apply([&map](auto&&... entities) mutable {
                (..., entities->base()->scheme_created(map));
            }, entities);

            // Call the creation branch of the callback
            tao::apply(std::move(always_callback), entities);
        });
    }

    template <typename I, template <typename...> typename S, typename PC, typename CC, typename AC, typename... A, typename... vecs>
    constexpr void create_with_pointer_precondition(S<vecs...>& scheme, PC&& precondition, CC&& created_callback, AC&& always_callback, A&&... scheme_args) noexcept
        requires (... && !std::is_lvalue_reference<A>::value)
    {
        static_assert(sizeof...(vecs) == sizeof...(scheme_args), "Incomplete scheme creation");

        schedule([
            this,
            &scheme,
            precondition = std::move(precondition),
            created_callback = std::move(created_callback),
            always_callback = std::move(always_callback),
            ... scheme_args = std::forward<A>(scheme_args)
        ] () mutable {
            auto pointer = precondition();
            if (pointer)
            {
                std::move(always_callback)(pointer);
                return;
            }

            uint64_t id = id_generator().next();
            // Create entities by using each allocator and arguments
            // Call callback now too
            auto entities = tao::apply(std::move(created_callback), tao::forward_as_tuple(create(id, scheme, std::move(scheme_args)) ...));

            // Create dynamic content
            auto map = std::make_shared<components_map>(entities);

            // Notify of complete scheme creation
            tao::apply([&map](auto&&... entities) mutable {
                (..., entities->base()->scheme_created(map));
                }, entities);

            // Call the creation branch of the callback
            std::move(always_callback)(tao::get<std::remove_pointer_t<I>*>(entities));
        });
    }

    template <typename I, template <typename...> typename S, typename PC, typename CC, typename AC, typename... A, typename... vecs>
    constexpr void create_with_pointer_precondition_and_id(uint64_t id, S<vecs...>& scheme, PC&& precondition, CC&& created_callback, AC&& always_callback, A&&... scheme_args) noexcept
        requires (... && !std::is_lvalue_reference<A>::value)
    {
        static_assert(sizeof...(vecs) == sizeof...(scheme_args), "Incomplete scheme creation");

        schedule([
            this,
            id,
            &scheme,
            precondition = std::move(precondition),
            created_callback = std::move(created_callback),
            always_callback = std::move(always_callback),
            ... scheme_args = std::forward<A>(scheme_args)
        ] () mutable {
            auto pointer = precondition();
            if (pointer)
            {
                std::move(always_callback)(pointer);
                return;
            }

            // Create entities by using each allocator and arguments
            // Call callback now too
            auto entities = tao::apply(std::move(created_callback), tao::forward_as_tuple(create(id, scheme, std::move(scheme_args)) ...));

            // Create dynamic content
            auto map = std::make_shared<components_map>(entities);

            // Notify of complete scheme creation
            tao::apply([&map](auto&&... entities) {
                (..., entities->base()->scheme_created(map));
                }, entities);

            // Call the creation branch of the callback
            std::move(always_callback)(tao::get<std::remove_pointer_t<I>*>(entities));
        });
    }

    inline generator& id_generator() noexcept
    {
        return _global_id_gen;
    }

private:
    template <template <typename...> typename S, typename... vecs, typename T>
    constexpr auto create(uint64_t id, S<vecs...>& scheme, T&& scheme_args) noexcept
    {
        // Create by invoking with arguments
        auto entity = tao::apply([&scheme_args, &id](auto&&... args) {
            return scheme_args.comp.alloc(id, std::forward<std::decay_t<decltype(args)>>(args)...);
        }, scheme_args.args);

        // Notify of creation
        entity->base()->scheme_information(scheme);

        return entity;
    }

    template <template <typename...> typename S, typename... vecs, typename T>
    constexpr auto create_with_partition(bool p, uint64_t id, S<vecs...>& scheme, T&& scheme_args) noexcept
    {
        // Create by invoking with arguments
        auto entity = tao::apply([&scheme_args, p, &id](auto&&... args) {
            return scheme_args.comp.alloc_with_partition(p, id, std::forward<std::decay_t<decltype(args)>>(args)...);
        }, scheme_args.args);

        // Notify of creation
        entity->base()->scheme_information(scheme);

        return entity;
    }

protected:
    bool _stop;

private:
    // Helpers
    generator _global_id_gen;

    // Workers
    std::vector<std::thread> _workers;
    boost::fibers::mutex _mutex;
    boost::fibers::condition_variable_any _cv;
};


using executor = base_executor<void>;

