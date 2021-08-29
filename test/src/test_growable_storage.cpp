#include <catch2/catch_all.hpp>
#include <random>

#include <entity/entity.hpp>
#include <entity/scheme.hpp>
#include <storage/growable_storage.hpp>


class client : public entity<client>
{
public:
    using entity<client>::entity;
};

constexpr uint32_t initial_size = 128;

template <typename T, uint32_t N>
using storage_base_t = growable_storage<T, N>;
using storage_type_t = storage_base_t<client, initial_size>;


template <typename S>
int push_simple(S& storage, uint64_t id = 0)
{
    if constexpr (has_storage_tag(storage_type_t::tag, storage_grow::none, storage_layout::partitioned))
    {
        storage.push(true, id);
        storage.push(false, id + 1);
        return 2;
    }
    else
    {
        storage.push(id);
        return 1;
    }
}

template <typename S>
void push_random_partition_if_available(S& storage, uint64_t id)
{
    if constexpr (has_storage_tag(storage_type_t::tag, storage_grow::none, storage_layout::partitioned))
    {
        static std::random_device rd;  //Will be used to obtain a seed for the random number engine
        static std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        static std::uniform_int_distribution<> distrib(0, 1);

        storage.push((bool)distrib(gen), id);
    }
    else
    {
        storage.push(id);
    }
}


SCENARIO("We can create and use storage type" + std::string(typeid(storage_type_t).name()), "[scheme]") {
    storage_type_t storage;

    GIVEN("The growable storage")
    {
        WHEN("Nothing is done")
        {
            THEN("The storage is empty")
            {
                REQUIRE(storage.size() == 0);
            }

            THEN("It can be iterated but no item is found")
            {
                bool empty = true;
                for (auto x : storage.range())
                {
                    empty = false;
                }
                REQUIRE(empty);
            }
        }

        WHEN("An item is allocated")
        {
            int count = push_simple(storage);

            THEN("The size increases")
            {
                REQUIRE(storage.size() == count);
            }

            THEN("It can be iterated and an item is found")
            {
                bool empty = true;
                for (auto x : storage.range())
                {
                    empty = false;
                }
                REQUIRE(!empty);
            }
        }

        WHEN("Many items are allocated without expanding the storage")
        {
            const int max_elements = initial_size - 5;
            for (int i = 0; i < max_elements; ++i)
            {
                push_random_partition_if_available(storage, i);
            }

            THEN("The size increases")
            {
                REQUIRE(storage.size() == max_elements);
            }

            THEN("It can be iterated and all items are found")
            {
                int count = 0;
                for (auto x : storage.range())
                {
                    ++count;
                }
                REQUIRE(count == max_elements);
            }
        }

        WHEN("Many items are allocated")
        {
            const int max_elements = has_storage_tag(storage_type_t::tag, storage_grow::fixed, storage_layout::none) ? initial_size : 612;
            for (int i = 0; i < max_elements; ++i)
            {
                push_random_partition_if_available(storage, i);
            }

            if constexpr (has_storage_tag(storage_type_t::tag, storage_grow::fixed, storage_layout::none))
            {
                REQUIRE(storage.full());
            }

            THEN("The size increases")
            {
                REQUIRE(storage.size() == max_elements);
            }

            THEN("It can be iterated and all items are found")
            {
                int count = 0;
                for (auto x : storage.range())
                {
                    ++count;
                }
                REQUIRE(count == max_elements);
            }
        }
    }
}

SCENARIO("We can create and use " + std::string(typeid(storage_type_t).name()) + " with orchestrators", "[scheme]") {
    orchestrator<storage_base_t, client, initial_size> orchestrator;

    GIVEN("The growable storage")
    {
        WHEN("Nothing is done")
        {
            THEN("The storage is empty")
            {
                REQUIRE(orchestrator.size() == 0);
            }

            THEN("It can be iterated but no item is found")
            {
                bool empty = true;
                for (auto x : orchestrator.range())
                {
                    empty = false;
                }
                REQUIRE(empty);
            }
        }

        WHEN("An item is allocated")
        {
            int count = push_simple(orchestrator);

            THEN("The size increases")
            {
                REQUIRE(orchestrator.size() == count);
            }

            THEN("It can be iterated and an item is found")
            {
                bool empty = true;
                for (auto x : orchestrator.range())
                {
                    empty = false;
                }
                REQUIRE(!empty);
            }
        }

        WHEN("Many items are allocated without expanding the orchestrator")
        {
            const int max_elements = initial_size - 5;
            for (int i = 0; i < max_elements; ++i)
            {
                push_random_partition_if_available(orchestrator, i);
            }

            THEN("The size increases")
            {
                REQUIRE(orchestrator.size() == max_elements);
            }

            THEN("It can be iterated and all items are found")
            {
                int count = 0;
                for (auto x : orchestrator.range())
                {
                    ++count;
                }
                REQUIRE(count == max_elements);
            }
        }

        WHEN("Many items are allocated")
        {
            const int max_elements = has_storage_tag(storage_type_t::tag, storage_grow::fixed, storage_layout::none) ? initial_size : 612;
            for (int i = 0; i < max_elements; ++i)
            {
                push_random_partition_if_available(orchestrator, i);
            }

            if constexpr (has_storage_tag(storage_type_t::tag, storage_grow::fixed, storage_layout::none))
            {
                REQUIRE(orchestrator.full());
            }

            THEN("The size increases")
            {
                REQUIRE(orchestrator.size() == max_elements);
            }

            THEN("It can be iterated and all items are found")
            {
                int count = 0;
                for (auto x : orchestrator.range())
                {
                    ++count;
                }
                REQUIRE(count == max_elements);
            }
        }


        for (int split = 0; split < 10; ++split)
        {
            WHEN("Many items are allocated and then randomly deleted (" + std::to_string(split) + "/10 times)")
            {
                orchestrator.clear();

                std::set<uint64_t> ids;
                const int max_elements = 1012;
                for (int i = 0; i < max_elements; ++i)
                {
                    if (i >= initial_size && has_storage_tag(storage_type_t::tag, storage_grow::fixed, storage_layout::none))
                    {
                        REQUIRE(orchestrator.full());
                        break;
                    }

                    push_random_partition_if_available(orchestrator, i);
                    ids.insert(i);
                }

                const int delete_expect = max_elements / 2;
                std::set<uint64_t> deleted_ids;
                std::sample(ids.begin(), ids.end(), std::inserter(deleted_ids, deleted_ids.begin()), delete_expect, std::mt19937{ std::random_device{}() });
                for (auto id : deleted_ids)
                {
                    REQUIRE(orchestrator.get(id) != nullptr);
                    orchestrator.pop(orchestrator.get(id));
                }

                THEN("The size matches the remaining elements")
                {
                    REQUIRE(orchestrator.size() == max_elements - deleted_ids.size());
                }

                THEN("It can be iterated and all items are found")
                {
                    int count = 0;
                    for (auto x : orchestrator.range())
                    {
                        ++count;
                    }
                    REQUIRE(count == max_elements - deleted_ids.size());
                }

                THEN("Entities in the deleted sample are not found")
                {
                    for (auto id : deleted_ids)
                    {
                        REQUIRE(orchestrator.get(id) == nullptr);
                    }
                }

                THEN("Iterating yields no deleted element")
                {
                    for (auto obj : orchestrator.range())
                    {
                        REQUIRE(!deleted_ids.contains(obj->id()));
                    }
                }

                THEN("Iterated IDs are unique")
                {
                    std::set<uint64_t> unique_ids;
                    for (auto obj : orchestrator.range())
                    {
                        REQUIRE(!unique_ids.contains(obj->id()));
                        unique_ids.insert(obj->id());
                    }
                }
            }
        }
    }
}
