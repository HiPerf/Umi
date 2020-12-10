#include "database/database.hpp"


void database::initialize(const mongocxx::uri& uri, std::string database, const std::unordered_map<uint8_t, std::string>& collections_map)
{
    instance = new class database(uri, database, collections_map);
}

database::database(const mongocxx::uri& uri, std::string database, const std::unordered_map<uint8_t, std::string>& collections_map) :
    _instance(),
    _pool(uri),
    _database(database),
    _collections_map(collections_map)
{}

mongocxx::database& database::get_database()
{
    thread_local auto client = _pool.acquire();
    thread_local auto database = client->database(_database);
    return database;
}

mongocxx::collection database::get_collection(uint8_t collection)
{
    auto& database = get_database();
    return database[_collections_map[collection]];
}
