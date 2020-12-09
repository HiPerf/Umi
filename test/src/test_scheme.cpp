#include <catch2/catch_all.hpp>

#include <entity/entity.hpp>
#include <entity/scheme.hpp>
#include <containers/dictionary.hpp>


class client : public entity<client>
{
public:
    using entity<client>::entity;
};

    
SCENARIO( "schemes can be created and used", "[scheme]" ) {

    GIVEN( "A scheme with duplicate types" ) {
        scheme_store<dictionary<client, entity<client>, 2>> store;

        auto client_scheme = scheme_maker<client, client>()(store);
        static_assert(std::is_same_v< decltype(client_scheme), scheme<dictionary<client, entity<client>, 2>> >, "Scheme type mismatch");

        client_scheme.require<client>();
        REQUIRE(client_scheme.has<client>());

        WHEN( "a non existent element is searched" ) {
            auto tuple = client_scheme.search(1);
            static_assert(tao::tuple_size<decltype(tuple)>() == 1);

            THEN( "the element is nullptr" ) {
                REQUIRE( tao::get<0>(tuple) == nullptr );
            }
        }

        WHEN( "entities are created" ) {
            uint64_t id = 1;
            auto scheme_args = client_scheme.args<client>();
            auto object = tao::apply([&scheme_args, id](auto&&... args) {
                return scheme_args.comp.alloc(id, std::forward<std::decay_t<decltype(args)>>(args)...);
            }, scheme_args.args);

            THEN( "the entity exists" ) {
                REQUIRE(object != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(1)) != nullptr);
                REQUIRE(tao::get<0>(client_scheme.search(1)) == object);
            }

            THEN( "the dictionary size increases" ) {
                REQUIRE(store.get<dictionary<client, entity<client>, 2>>().size() == 1);
                REQUIRE(client_scheme.get<client>().size() == 1);
            }
        }
    }
}
