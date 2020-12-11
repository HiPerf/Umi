#include <catch2/catch_all.hpp>

#include <tao/tuple/tuple.hpp>
#include <tao/seq/integer_sequence.hpp>
#include <database/database.hpp>

#include <unordered_set>


using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::kvp;


SCENARIO("database unique id generation") {
    GIVEN("A database") {
        database::initialize(mongocxx::uri("mongodb://localhost"), "umi-test", {});

        WHEN("unique ids are requested") {
            std::unordered_set<uint64_t> ids;
            uint64_t total_cases = 100000;

            for (uint64_t i = 0; i < total_cases; ++i)
            {
                ids.insert(database::instance->get_unique_id());
            }

            THEN("There are no collisions") {
                REQUIRE(ids.size() / static_cast<double>(total_cases) > 0.99);
            }
        }

        WHEN("a model is modified") {
            auto model = mongocxx::model::insert_one(make_document(
                kvp("key", "value")
            ));

            auto document = make_document(kvp("_id", "random_id"), bsoncxx::builder::concatenate(model.document().view()));
            THEN("The document is as expected") {
                auto view = document.view();
                REQUIRE(view.find("key") != view.end());
                REQUIRE(view["key"].get_utf8().value == "value");

                REQUIRE(view.find("_id") != view.end());
                REQUIRE(view["_id"].get_utf8().value == "random_id");
            }
        }

        WHEN("a model is modified two times") {
            auto model = mongocxx::model::insert_one(make_document(
                kvp("key", "value")
            ));

            auto document1 = make_document(kvp("_id", "random_id"), bsoncxx::builder::concatenate(model.document().view()));
            auto document2 = make_document(kvp("_id", "random_id_2"), bsoncxx::builder::concatenate(model.document().view()));
            THEN("The document is as expected") {
                auto view = document2.view();
                REQUIRE(view.find("key") != view.end());
                REQUIRE(view["key"].get_utf8().value == "value");

                REQUIRE(view.find("_id") != view.end());
                REQUIRE(view["_id"].get_utf8().value == "random_id_2");
            }
        }

        WHEN("a model with id is modified") {
            auto model = mongocxx::model::insert_one(make_document(
                kvp("_id", "previous"),
                kvp("key", "value")
            ));

            auto document = make_document(kvp("_id", "random_id"), bsoncxx::builder::concatenate(model.document().view()));
            THEN("The document is as expected") {
                auto view = document.view();
                REQUIRE(view.find("key") != view.end());
                REQUIRE(view["key"].get_utf8().value == "value");

                REQUIRE(view.find("_id") != view.end());
                REQUIRE(view["_id"].get_utf8().value == "random_id");
            }
        }
    }
}
