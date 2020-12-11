#pragma once

#include "common/types.hpp"

#include <mongocxx/bulk_write.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>

#include <set>
#include <optional>
#include <unordered_map>



class database
{
public:
    static inline database* instance = nullptr;

    static void initialize(const mongocxx::uri& uri, std::string database, const std::unordered_map<uint8_t, std::string>& collections_map);

    uint64_t ensure_creation(uint8_t collection, mongocxx::model::insert_one&& op);

    mongocxx::collection get_collection(uint8_t collection);

    uint64_t get_unique_id();

private:
    database(const mongocxx::uri& uri, std::string database, const std::unordered_map<uint8_t, std::string>& collections_map);

    mongocxx::database& get_database();

private:
    mongocxx::instance _instance;
    mongocxx::pool _pool;
    std::string _database;

    std::unordered_map<uint8_t, std::string> _collections_map;
};
