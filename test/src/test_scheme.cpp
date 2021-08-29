#include <catch2/catch_all.hpp>

#include <entity/entity.hpp>
#include <entity/scheme.hpp>
#include <containers/dictionary.hpp>


class client : public entity<client>
{
public:
    using entity<client>::entity;
};

template <typename S>
client* alloc_one(uint64_t id, S& scheme)
{
    auto scheme_args = scheme.template args<client>();
    return tao::apply([&scheme_args, id](auto&&... args) {
        return scheme_args.comp.alloc(id, std::forward<std::decay_t<decltype(args)>>(args)...);
    }, scheme_args.args);
}

/*
SCENARIO( "schemes can be created and used", "[scheme]" ) {

    GIVEN( "A scheme with duplicate types" ) {
        using pool_t = dictionary<client, entity<client>, 2>;

        scheme_store<pool_t> store;
        auto& x = tao::get<0>(store.components);
        auto client_scheme = scheme_maker<client, client>()(store);
        static_assert(std::is_same_v< decltype(client_scheme), scheme<pool_t> >, "Scheme type mismatch");

        client_scheme.require<client>();
        REQUIRE(client_scheme.has<client>());

        WHEN( "the inner pool is requested" ) {
            THEN( "it matches the store type" ) {
                REQUIRE( &client_scheme.get<client>() == &tao::get<0>(store.components) );
                REQUIRE( &client_scheme.get<pool_t>() == &tao::get<0>(store.components) );
                REQUIRE( &client_scheme.get<pool_t>() == &store.get<pool_t>() );
            }
        }

        WHEN( "a non existent element is searched" ) {
            auto tuple = client_scheme.search(1);       
            static_assert(tao::tuple_size<decltype(tuple)>() == 1);

            THEN( "the element is nullptr" ) {
                REQUIRE( tao::get<0>(tuple) == nullptr );
            }
        }

        WHEN( "one entity is created" ) {
            uint64_t id = 1;
            auto object = alloc_one(id, client_scheme);

            THEN( "the entity exists" ) {
                REQUIRE(object != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(id)) != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(id)) == object);
            }

            THEN( "the dictionary size increases" ) {
                REQUIRE(store.get<pool_t>().size() == 1);
                REQUIRE(client_scheme.get<client>().size() == 1);
            }

            THEN( "static size has increased, but not dynamic" ) {
                REQUIRE(client_scheme.get<client>().is_static(object));
                REQUIRE(!client_scheme.get<client>().is_static_full());
            }

            THEN( "the entity ticket points to the returned object" ) {
                REQUIRE(object->ticket()->get() == object);
            }
        }

        WHEN( "two entities are created" ) {
            auto object1 = alloc_one(1, client_scheme);
            auto object2 = alloc_one(2, client_scheme);

            THEN( "the entity exists" ) {
                REQUIRE(object1 != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(1)) != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(1)) == object1);
                
                REQUIRE(object2 != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(2)) != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(2)) == object2);
            }

            THEN( "the dictionary size increases" ) {
                REQUIRE(store.get<pool_t>().size() == 2);
                REQUIRE(client_scheme.get<client>().size() == 2);
            }

            THEN( "static is full" ) {
                REQUIRE(client_scheme.get<client>().is_static(object1));
                REQUIRE(client_scheme.get<client>().is_static(object2));
                REQUIRE(client_scheme.get<client>().is_static_full());
            }
        }

        WHEN( "three entities are created" ) {
            auto object1 = alloc_one(1, client_scheme);
            auto object2 = alloc_one(2, client_scheme);
            auto object3 = alloc_one(3, client_scheme);

            THEN( "the entity exists" ) {
                REQUIRE(object1 != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(1)) != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(1)) == object1);
                
                REQUIRE(object2 != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(2)) != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(2)) == object2);
                
                REQUIRE(object3 != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(3)) != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(3)) == object3);
            }

            THEN( "the dictionary size increases" ) {
                REQUIRE(store.get<pool_t>().size() == 3);
                REQUIRE(client_scheme.get<client>().size() == 3);
            }

            THEN( "static is full and last object is dynamic" ) {
                REQUIRE(client_scheme.get<client>().is_static_full());
                REQUIRE(client_scheme.get<client>().is_static(object1));
                REQUIRE(client_scheme.get<client>().is_static(object2));
                REQUIRE(!client_scheme.get<client>().is_static(object3));
            }
        }

        WHEN( "one entity is added and then removed" ) {
            auto object1 = alloc_one(1, client_scheme);
            auto ticket = object1->ticket();
            client_scheme.get<client>().free(object1);

            THEN( "the ticket is no longer valid " ) {
                REQUIRE(object1 != nullptr);
                REQUIRE(!ticket->valid());
                REQUIRE(ticket->get() == nullptr);
            }

            THEN( "the object can no longer be found" ) {
                REQUIRE(tao::get<0>(client_scheme.search(1)) == nullptr);
            }

            THEN( "the dictionary size is 0" ) {
                REQUIRE(store.get<pool_t>().size() == 0);
            }
        }

        WHEN( "two entities are added and then the first is removed" ) {
            auto object1 = alloc_one(1, client_scheme);
            auto ticket1 = object1->ticket();

            auto object2 = alloc_one(2, client_scheme);
            auto ticket2 = object2->ticket();

            client_scheme.get<client>().free(object1);

            THEN( "the second entity has been moved to the memory space of the first" ) {
                REQUIRE(ticket2->get() == object1);
            }

            THEN( "the first object can no longer be found, but the second can" ) {
                REQUIRE(tao::get<0>(client_scheme.search(1)) == nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(2)) != nullptr);
            }

            THEN( "the dictionary size is 0" ) {
                REQUIRE(store.get<pool_t>().size() == 1);
            }
        }
    }
}*/
