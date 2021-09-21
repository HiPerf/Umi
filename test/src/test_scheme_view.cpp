#include <catch2/catch_all.hpp>

#include <entity/entity.hpp>
#include <entity/scheme.hpp>
#include <storage/growable_storage.hpp>
#include <storage/partitioned_growable_storage.hpp>
#include <storage/partitioned_static_storage.hpp>
#include <storage/static_growable_storage.hpp>
#include <storage/static_storage.hpp>
#include <view/scheme_view.hpp>


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
void test_iteration_with_single_storage()
{
    GIVEN("a " + std::string(typeid(S<client, 128>).name()) + " store and a scheme with two components")
    {
        scheme_store<
            S<client, 128>,
            S<npc, 128>
        > store;

        auto scheme = scheme_maker<client, npc>()(store);

        WHEN("two entities are created in the scheme")
        {
            for (int i = 0; i < 2; ++i)
            {
                scheme.create(i, get_args<client, S<client, 128>>(scheme), get_args<npc, S<npc, 128>>(scheme));
            }

            THEN("they can be iterated continuously with a view")
            {
                waitable waitable;

                auto idx = 0;
                scheme_view::continuous(waitable, scheme, [&idx](auto client, auto npc) 
                    {
                        REQUIRE(std::is_same_v<decltype(client), class client*>);
                        REQUIRE(std::is_same_v<decltype(npc), class npc*>);

                        REQUIRE(client->id() == idx);
                        REQUIRE(npc->id() == idx);
                        idx += 1;
                    });

                REQUIRE(waitable.done());
                REQUIRE(idx == 2);
            }

            THEN("they can be iterated continuously with a view")
            {
                waitable waitable;

                auto idx = 0;
                scheme_view::parallel(waitable, scheme, [&idx](auto client, auto npc) 
                    {
                        REQUIRE(std::is_same_v<decltype(client), class client*>);
                        REQUIRE(std::is_same_v<decltype(npc), class npc*>);

                        REQUIRE(client->id() == idx);
                        REQUIRE(npc->id() == idx);
                        idx += 1;
                    });

                REQUIRE(waitable.done());
                REQUIRE(idx == 2);
            }
        }
    }
}
//
//template <template <typename, uint32_t> typename S1, template <typename, uint32_t> typename S2>
//void test_iteration_with_single_storage()
//{
//    GIVEN("a " + std::string(typeid(S1<client, 128>).name()) + " store, a " + std::string(typeid(S2<client, 128>).name()) + " store and a scheme with two components")
//    {
//        scheme_store<
//            S1<client, 128>,
//            S2<npc, 128>
//        > store;
//
//        if constexpr (is_partitioned(S1<client, 128>::tag))
//        {
//            //GIVEN()
//        }
//    }
//}

SCENARIO("schemes can be iterated with scheme views")
{
    test_iteration_with_single_storage<growable_storage>();
    test_iteration_with_single_storage<partitioned_growable_storage>();
    test_iteration_with_single_storage<partitioned_static_storage>();
    test_iteration_with_single_storage<static_growable_storage>();
    test_iteration_with_single_storage<static_storage>();
}

