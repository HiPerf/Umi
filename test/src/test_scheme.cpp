#include <catch2/catch_all.hpp>

#include <entity/entity.hpp>
#include <entity/scheme.hpp>
#include <storage/growable_storage.hpp>
#include <storage/partitioned_growable_storage.hpp>
#include <storage/partitioned_static_storage.hpp>
#include <storage/static_growable_storage.hpp>
#include <storage/static_storage.hpp>


class client : public entity<client>
{
public:
    using entity<client>::entity;

    void construct()
    {
        constructor_called = false;
    }

    void construct(int i)
    {
        constructor_called = true;
    }
    
    bool constructor_called = false;
};

class npc : public entity<npc>
{
public:
    using entity<npc>::entity;
};

class invalid_component
{};

class non_registered_component : public entity<non_registered_component>
{
public:
    using entity<non_registered_component>::entity;
};


SCENARIO("orchestrators can be derived from storages", "[scheme]")
{
    GIVEN("basic components")
    {
        using T = orchestrator_type<entity<client>, growable_storage<client, 128>>;
        using D = orchestrator_type<client, growable_storage<client, 128>>;
        using E = orchestrator_type<growable_storage<client, 128>, growable_storage<client, 128>>;

        // Check types
        THEN("types match")
        {
            REQUIRE(std::is_same_v<T, growable_storage<client, 128>::orchestrator_t>);
            REQUIRE(std::is_same_v<D, growable_storage<client, 128>::orchestrator_t>);
            REQUIRE(std::is_same_v<E, growable_storage<client, 128>::orchestrator_t>);
        }
    }
}

template <template <typename, uint32_t> typename S>
void test_scheme_creation_with_storage()
{
    GIVEN("type " + std::string(typeid(S).name()))
    {
        THEN("we can create stores with one component")
        {
            scheme_store<S<client, 128>> store;

            // All three methods work
            //  Note: We use & to get a pointer, but the compiler would error in case either of theese was wrong
            REQUIRE(&store.get<client>());
            REQUIRE(&store.get<S<client, 128>>());
            REQUIRE(&store.get<S<client, 128>::orchestrator_t>());

            // Check returned types
            // TODO(gpascualg): MSVC fails to parse S<client, 128>::... in the next line
            //REQUIRE(std::is_same_v<decltype(store.get<client>()), S<client, 128>::orchestrator_t&>);
        }

        THEN("we can create stores with multiple components")
        {
            scheme_store<
                S<client, 128>,
                S<npc, 128>
            > store;

            REQUIRE(&store.get<client>());
            REQUIRE(&store.get<npc>());
        }
    }

    GIVEN("one " + std::string(typeid(S).name()) + " store with two components")
    {
        scheme_store<
            S<client, 128>,
            S<npc, 128>
        > store;

        THEN("schemes with one component can be manually created")
        {
            scheme<S<client, 128>::orchestrator_t> client_scheme(store);

            REQUIRE(client_scheme.has<client>());
            REQUIRE(!client_scheme.has<npc>());
            REQUIRE(!client_scheme.has<non_registered_component>());
            REQUIRE(!client_scheme.has<invalid_component>());

            client_scheme.require<client>();
        }

        THEN("schemes with multiple components can be manually created")
        {
            scheme<
                S<client, 128>::orchestrator_t,
                S<npc, 128>::orchestrator_t
            > client_scheme(store);

            REQUIRE(client_scheme.has<client>());
            REQUIRE(client_scheme.has<npc>());
            REQUIRE(!client_scheme.has<non_registered_component>());
            REQUIRE(!client_scheme.has<invalid_component>());

            client_scheme.require<client>();
            client_scheme.require<npc>();
        }

        THEN("a scheme with one component can be created through scheme makers")
        {
            auto scheme = scheme_maker<client>()(store);

            REQUIRE(scheme.has<client>());
            REQUIRE(!scheme.has<npc>());
            REQUIRE(!scheme.has<non_registered_component>());
            REQUIRE(!scheme.has<invalid_component>());

            scheme.require<client>();
        }

        THEN("a scheme with multiple components can be created through scheme makers")
        {
            auto scheme = scheme_maker<client, npc>()(store);

            REQUIRE(scheme.has<client>());
            REQUIRE(scheme.has<npc>());
            REQUIRE(!scheme.has<non_registered_component>());
            REQUIRE(!scheme.has<invalid_component>());

            scheme.require<client>();
            scheme.require<npc>();
        }

        THEN("a scheme with all components can be created through scheme makers")
        {
            auto scheme = scheme_maker()(store);

            REQUIRE(scheme.has<client>());
            REQUIRE(scheme.has<npc>());
            REQUIRE(!scheme.has<non_registered_component>());
            REQUIRE(!scheme.has<invalid_component>());

            scheme.require<client>();
            scheme.require<npc>();
        }
    }
}

template <typename T, typename O, typename S, typename... Args>
auto get_args(S& scheme, Args&&... args)
{
    if constexpr (is_partitioned_storage(O::tag))
    {
        return scheme.args<T>(true, std::forward<Args>(args)...);
    }
    else
    {
        return scheme.args<T>(std::forward<Args>(args)...);
    }
}

template <template <typename, uint32_t> typename S>
void test_instantiation_with_storage()
{
    GIVEN("a " + std::string(typeid(S).name()) + " store and a scheme with two components")
    {
        scheme_store<
            S<client, 128>,
            S<npc, 128>
        > store;

        auto scheme = scheme_maker<client, npc>()(store);

        WHEN("a component is allocated in the scheme without any parameters")
        {
            auto component = scheme.alloc<client>(1, get_args<client, S<client, 128>>(scheme));

            THEN("the component is valid")
            {
                REQUIRE(component != nullptr);
                REQUIRE(std::is_same_v<decltype(component), client*>);
                REQUIRE(component->id() == 1);
            }

            THEN("constructor has not been called")
            {
                REQUIRE(!component->constructor_called);
            }
        }

        WHEN("a component is allocated in the scheme with parameters")
        {
            auto component = scheme.alloc<client>(1, get_args<client, S<client, 128>>(scheme, 1));

            THEN("the component is valid")
            {
                REQUIRE(component != nullptr);
                REQUIRE(std::is_same_v<decltype(component), client*>);
                REQUIRE(component->id() == 1);
            }

            THEN("constructor has been called")
            {
                REQUIRE(component->constructor_called);
            }
        }

        WHEN("two components are allocated in the scheme")
        {
            auto component1 = scheme.alloc<client>(1, get_args<client, S<client, 128>>(scheme));
            auto component2 = scheme.alloc<client>(2, get_args<client, S<client, 128>>(scheme));

            THEN("the component is valid")
            {
                REQUIRE(component1 != nullptr);
                REQUIRE(component2 != nullptr);
                REQUIRE(component1 != component2);
            }
        }

        WHEN("an entity is created at once")
        {
            auto entity = scheme.create(1,
                get_args<client, S<client, 128>>(scheme),
                get_args<npc, S<npc, 128>>(scheme));

            THEN("the entity has valid components")
            {
                REQUIRE(tao::get<client*>(entity) != nullptr);
                REQUIRE(tao::get<npc*>(entity) != nullptr);
                REQUIRE(tao::get<client*>(entity)->id() == 1);
                REQUIRE(tao::get<npc*>(entity)->id() == 1);
            }
        }
    }
}

template <template <typename, uint32_t> typename S>
void test_destruction_with_storage()
{
    GIVEN("a " + std::string(typeid(S).name()) + " store and a scheme with two components")
    {
        scheme_store<
            S<client, 128>,
            S<npc, 128>
        > store;

        auto scheme = scheme_maker<client, npc>()(store);

        WHEN("an entity is created and then freed")
        {
            auto entity = scheme.create(1,
                get_args<client, S<client, 128>>(scheme),
                get_args<npc, S<npc, 128>>(scheme));

            scheme.destroy(entity);

            THEN("the entity no longer exists")
            {
                REQUIRE(scheme.size() == 0);
            }
        }

        WHEN("two entities are created and the first is freed")
        {
            auto entity = scheme.create(1,
                get_args<client, S<client, 128>>(scheme),
                get_args<npc, S<npc, 128>>(scheme));

            auto other = scheme.create(2,
                get_args<client, S<client, 128>>(scheme),
                get_args<npc, S<npc, 128>>(scheme));

            auto tickets = entity.tickets();
            auto other_tickets = other.tickets();

            scheme.destroy(entity);

            THEN("the entity no longer exists")
            {
                REQUIRE(!tao::get<0>(tickets)->valid());
                REQUIRE(!tao::get<1>(tickets)->valid());

                REQUIRE(!tickets.valid<client>());
                REQUIRE(!tickets.valid<npc>());

                REQUIRE(scheme.size() == 1);
            }

            THEN("the second entity's ptr has changed")
            {
                REQUIRE(tao::get<0>(other_tickets)->valid());
                REQUIRE(tao::get<1>(other_tickets)->valid());

                REQUIRE(other_tickets.valid<client>());
                REQUIRE(other_tickets.valid<npc>());

                REQUIRE(tao::get<client*>(other) != other_tickets.get<client>());
                REQUIRE(other_tickets.get<client>()->id() == 2);
            }
        }

        WHEN("two entities are created and the second is freed")
        {
            auto entity = scheme.create(1,
                get_args<client, S<client, 128>>(scheme),
                get_args<npc, S<npc, 128>>(scheme));

            auto other = scheme.create(2,
                get_args<client, S<client, 128>>(scheme),
                get_args<npc, S<npc, 128>>(scheme));

            auto ticket = tao::get<client*>(entity)->ticket();
            auto other_ticket = tao::get<client*>(other)->ticket();

            scheme.destroy(other);

            THEN("the entity no longer exists")
            {
                REQUIRE(!other_ticket->valid());
                REQUIRE(scheme.size() == 1);
            }

            THEN("the second entity's ptr has not changed, as the vector remains untouched")
            {
                REQUIRE(ticket->valid());
                REQUIRE(tao::get<client*>(entity) == ticket->get());
                REQUIRE(ticket->get()->id() == 1);
            }
        }
    }
}

SCENARIO("schemes can be created", "[scheme]") 
{
    test_scheme_creation_with_storage<growable_storage>();
    test_scheme_creation_with_storage<partitioned_growable_storage>();
    test_scheme_creation_with_storage<partitioned_static_storage>();
    test_scheme_creation_with_storage<static_growable_storage>();
    test_scheme_creation_with_storage<static_storage>();
}

SCENARIO("schemes can be used to instantiate entities")
{
    test_instantiation_with_storage<growable_storage>();
    test_instantiation_with_storage<partitioned_growable_storage>();
    test_instantiation_with_storage<partitioned_static_storage>();
    test_instantiation_with_storage<static_growable_storage>();
    test_instantiation_with_storage<static_storage>();
}

SCENARIO("schemes can be used to instantiate entities and free them")
{
    test_destruction_with_storage<growable_storage>();
    test_destruction_with_storage<partitioned_growable_storage>();
    test_destruction_with_storage<partitioned_static_storage>();
    test_destruction_with_storage<static_growable_storage>();
    test_destruction_with_storage<static_storage>();
}

