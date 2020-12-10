#include "database/database.hpp"
#include "database/pseudorandom_unique_id.hpp"


using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::kvp;


void database::initialize(const mongocxx::uri& uri, std::string database, const std::unordered_map<uint8_t, std::string>& collections_map)
{
    if (instance == nullptr)
    {
        instance = new class database(uri, database, collections_map);
    }
}

database::database(const mongocxx::uri& uri, std::string database, const std::unordered_map<uint8_t, std::string>& collections_map) :
    _instance(),
    _pool(uri),
    _database(database),
    _collections_map(collections_map)
{}

uint64_t database::ensure_creation(uint8_t collection, mongocxx::model::insert_one&& op)
{
    auto col = get_collection(collection);

    while (true)
    {
        uint64_t id = get_unique_id();
        auto document = make_document(kvp("_id", static_cast<int64_t>(id)), bsoncxx::builder::concatenate(op.document().view()));
        if (col.insert_one(document.view()))
        {
            return id;
        }
    }
}

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

uint64_t database::get_unique_id()
{
    thread_local pseudorandom_unique_id gen;
    return gen.next();
}
