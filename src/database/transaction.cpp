#include "database/transaction.hpp"
#include "database/database.hpp"
#include "containers/static_store.hpp"


transaction::collection_info::collection_info() :
    first_id(0),
    current_id(0)
{}

void transaction::construct()
{
    _collections.clear();
}

void transaction::update(store_t* store)
{
    // Transactions are pending when ids don't match
    for (auto& [collection, info] : _collections)
    {
        if (info.first_id != info.current_id)
        {
            auto initial_id = info.first_id;
            std::vector<mongocxx::model::write> transactions = get_pending_operations(collection, store);
            auto final_id = info.first_id;
            
            if (transactions.empty())
            {
                continue;
            }

            auto col = database::instance->get_collection(collection);
            auto bulk = col.create_bulk_write();
            
            for (auto& t : transactions)
            {
                bulk.append(t);
            }

            // Send transactions
            bulk.execute();

            // Once we get here, they are all executed, so flag them
            for (; initial_id != final_id; ++initial_id)
            {
                info.transactions[initial_id].done = true;
                info.transactions[initial_id].pending = false;
            }
        }
    }
}

uint64_t transaction::push_operation(uint8_t collection, mongocxx::model::write&& operation)
{
    collection_info& info = _collections[collection];
    uint64_t slot = ++info.current_id;
    transaction_info* transaction = &info.transactions[slot];

    transaction->dependency = std::nullopt;
    transaction->operation = std::move(operation);
    transaction->done = false;
    transaction->pending = false;

    return slot;
}

void transaction::push_dependency(uint8_t collection, uint64_t owner, uint64_t id)
{
    collection_info& info = _collections[collection];
    uint64_t slot = ++info.current_id;
    transaction_info* transaction = &info.transactions[slot];

    transaction->dependency = { .owner = owner, .id = id };
    transaction->operation = std::nullopt;
    transaction->done = false;
    transaction->pending = false;
}

std::vector<mongocxx::model::write> transaction::get_pending_operations(uint8_t collection, static_store<transaction, entity<transaction>>* store)
{
    collection_info& info = _collections[collection];
    std::vector<mongocxx::model::write> operations;
    std::set<uint64_t> ids;

    uint64_t id = info.first_id;
    for (; id != info.current_id; ++id)
    {
        auto& transaction = info.transactions[id];

        // This transaction has been sent and is still pending
        if (transaction.pending)
        {
            break;
        }

        if (transaction.done)
        {
            // TODO(gpascualg): Log why should this case happen at all
            continue;
        }

        if (transaction.dependency)
        {
            auto dependency = *transaction.dependency;
            if (auto other = store->get_derived_or_null(dependency.owner))
            {
                if (auto info = other->get_transaction(collection, dependency.id))
                {
                    if (!info->done)
                    {
                        // Stop here if there is a dependency that has not yet completed
                        break;
                    }
                }
            }
        }

        transaction.pending = true;

        // Some transactions might be simple deps without op
        if (transaction.operation)
        {
            operations.push_back(std::move(*transaction.operation));
        }
    }

    info.first_id = id;
    return operations;
}

transaction::transaction_info* transaction::get_transaction(uint8_t collection, uint64_t id)
{
    if (auto at = _collections.find(collection); at != _collections.end())
    {
        auto& info = at->second;
        if (auto it = info.transactions.find(id); it != info.transactions.end())
        {
            return &it->second;
        }
    }

    return nullptr;
}
