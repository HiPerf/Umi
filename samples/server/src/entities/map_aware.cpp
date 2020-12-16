#include "core/server.hpp"
#include "common/definitions.hpp"
#include "entities/map_aware.hpp"
#include "entities/transform.hpp"
#include "database/database.hpp"
#include "database/transaction.hpp"
#include "maps/region.hpp"
#include "maps/map.hpp"


using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::kvp;


void map_aware::construct(int64_t db_id)
{
    _db_id = db_id;
}

void map_aware::destroy()
{
    auto transform = get<class transform>();
    auto transaction = get<class transaction>();
    auto position = transform->position(server::instance->now());

    transaction->push_operation(static_cast<uint8_t>(database_collections::characters), mongocxx::model::update_one(
        make_document(kvp("_id", _db_id)),
        make_document(kvp("position", make_document(
            kvp("map", static_cast<int64_t>(transform->current_region()->get_map()->id())),
            kvp("x", position.x),
            kvp("y", position.y),
            kvp("z", position.z)
        )))
    ));
}
