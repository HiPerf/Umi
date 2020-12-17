#include "core/server.hpp"
#include <database/database.hpp>

#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

#include <glm/gtx/string_cast.hpp>

#include <iostream>


using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::kvp;


int main()
{
    database::initialize(mongocxx::uri("mongodb://localhost"), "umi", { 
        { static_cast<uint8_t>(database_collections::accounts), "accounts" },
        { static_cast<uint8_t>(database_collections::characters), "characters" }
    });

    // Unlog everyone
    // TODO(gpascualg): Not the best place to do logouts
    database::instance->get_collection(static_cast<uint8_t>(database_collections::accounts)).update_many(
        make_document(),
        make_document(kvp("$set", make_document(
            kvp("logged", false)
        )))
    );

    server server(7575, 3, 2);
    server.mainloop();
    server.stop();
    return 0;
}
