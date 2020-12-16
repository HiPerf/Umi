#include "core/handler.hpp"
#include "core/server.hpp"
#include "common/definitions.hpp"
#include "database/database.hpp"
#include "maps/map.hpp"

#include <kumo/config.hpp>
#include <kumo/rpc.hpp>


using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::kvp;



bool handler::handle_client_error(::kaminari::basic_client* kaminari_client, ::kumo::opcode opcode)
{
    return false;
}

bool handler::check_client_status(::kaminari::basic_client* kaminari_client, ingame_status status)
{
    auto client = (class client*)kaminari_client;
    return client->ingame_status() == status;
}

bool handler::on_move(::kaminari::basic_client* kaminari_client, const ::kumo::movement& data, uint64_t timestamp)
{
    return true;
}

bool handler::on_handshake(::kaminari::basic_client* kaminari_client, const ::kumo::client_handshake& data, uint64_t timestamp)
{
    auto client = (class client*)kaminari_client;
    client->handshake_done();

    bool matching_version = data.version == ::kumo::VERSION;
    kumo::send_handshake_response(client->super_packet(), { .success = matching_version });

    // Only accept latest protocol version
    if (!matching_version)
    {
        server::instance->disconnect_client(client);
    }

    return true;
}

bool handler::on_login(::kaminari::basic_client* kaminari_client, const ::kumo::login_data& data, uint64_t timestamp)
{
    auto client = (class client*)kaminari_client;
    client->login_pending();
    
    server::instance->database_async().submit([data, ticket = client->ticket()]() mutable
        {
            if (!ticket->valid())
            {
                return;
            }
            auto client = ticket->get()->derived();

            auto filter = make_document(
                kvp("_id", data.username),
                kvp("password0", static_cast<int64_t>(data.password0)),
                kvp("password1", static_cast<int64_t>(data.password1)),
                kvp("password2", static_cast<int64_t>(data.password2)),
                kvp("password3", static_cast<int64_t>(data.password3))
            );

            auto update = make_document(
                kvp("$set", make_document(
                    kvp("logged", true)
                ))
            );

            auto collection = database::instance->get_collection(static_cast<uint8_t>(database_collections::accounts));
            auto result = collection.find_one_and_update(filter.view(), update.view());
            if (!result)
            {
                server::instance->schedule_if(ticket, [](class client* client)
                    {
                        kumo::send_login_response(client->super_packet(), { .code = 1 });
                        client->handshake_done();
                    });
            }
            else
            {
                // The query returns the document before updating
                auto user = result->view();
                if (user["logged"].get_bool().value)
                {
                    server::instance->schedule_if(ticket, [](class client* client)
                        {
                            kumo::send_login_response(client->super_packet(), { .code = 2 });
                            client->handshake_done();
                        });
                    return;
                }

                client->database_information({
                    .username = data.username
                });
                client->login_done();

                auto characters_collection = database::instance->get_collection(static_cast<uint8_t>(database_collections::characters));
                std::vector<kumo::character> characters;

                for (auto& character : characters_collection.find(make_document(kvp("username", data.username))))
                {
                    characters.push_back(kumo::character {
                        .name = std::string(character["name"].get_utf8().value),
                        .level = static_cast<uint16_t>(character["level"].get_int32().value)
                    });
                }

                // Send login response with characters
                server::instance->schedule_if(ticket, [characters{ std::move(characters) }](class client* client) mutable
                    {
                        kumo::send_login_response(client->super_packet(), { .code = 0 });
                        kumo::send_characters_list(client->super_packet(), { .list = std::move(characters) });
                    });
            }
        });

    return true;
}

bool handler::on_character_selected(::kaminari::basic_client* kaminari_client, const ::kumo::character_selection& data, uint64_t timestamp)
{
    auto client = (class client*)kaminari_client;

    server::instance->database_async().submit([data, ticket = client->ticket()]() mutable
    {
        if (!ticket->valid())
        {
            return;
        }
        auto client = ticket->get()->derived();

        auto filter = make_document(
            kvp("username", client->database_information()->username),
            kvp("index", data.index)
        );

        auto collection = database::instance->get_collection(static_cast<uint8_t>(database_collections::characters));
        auto result = collection.find_one(filter.view());
        if (!result)
        {
            // Selected a non-existing character
            server::instance->disconnect_client(client);
            return;
        }

        // Load map and create character
        auto& character = result->view();
        server::instance->get_or_create_map(static_cast<uint64_t>(character["position"]["map"].get_int64().value), [
            ticket,
            id = static_cast<uint64_t>(character["_id"].get_int64().value),
            position = glm::vec3(
                static_cast<float>(character["position"]["x"].get_double().value), 
                static_cast<float>(character["position"]["y"].get_double().value),
                static_cast<float>(character["position"]["z"].get_double().value))
        ](map* map) mutable
            {
                map->create_entity_at(id, position, [ticket](auto map_aware, auto transform) mutable
                    {
                        if (!ticket->valid())
                        {
                            transform->current_region()->remove_entity(transform);
                            return;
                        }

                        auto client = ticket->get()->derived();
                        client->ingame_entity(transform);
                        kumo::send_enter_world(client->super_packet(), {});
                    });
            });
    });

    return true;
}
