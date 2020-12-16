#include "core/server.hpp"

#include "database/transaction.hpp"
#include "database/database.hpp"
#include "async/async_executor.hpp"
#include "containers/static_store.hpp"
#include "updater/executor_registry.hpp"


transaction::collection_info::collection_info() :
    first_id(0),
    current_id(0)
{}

transaction::transaction() noexcept
{
    // Avoid race conditions by creating now the whole set of collections
    _collections.try_emplace(static_cast<uint8_t>(database_collections::accounts));
    _collections.try_emplace(static_cast<uint8_t>(database_collections::characters));
}

void transaction::construct(uint64_t execute_every)
{
    _collections.clear();
    _execute_every = execute_every;
    _since_last_execution = 0;
    _pending_callables = 0;
    _flagged = false;
    _scheduled = false;
}

void transaction::update(uint64_t diff, store_t* store, async_executor_base* async)
{
    if (_flagged)
    {
        if (_scheduled)
        {
            return;
        }

        bool can_delete = true;

        // We will only delete if all transactions are done
        for (auto& [collection, info] : _collections)
        {
            if (info.first_id != info.current_id || !info.transactions[info.current_id].done)
            {
                can_delete = false;
                break;
            }
        }

        if (can_delete)
        {
            server::instance->schedule_entity_transaction_removal(this);
            _scheduled = true;
        }
    }

    // We have to execute if
    //  a) Too much time has elapsed
    //  b) There are pending callables
    _since_last_execution += diff;
    if (_pending_callables == 0 && _since_last_execution < _execute_every)
    {
        return;
    }
    _since_last_execution = 0;

    // Transactions are pending when ids don't match
    for (auto& [collection, info] : _collections)
    {
        if (info.first_id != info.current_id)
        {
            bool has_non_callable_transactions;
            std::vector<transaction_info*> transactions = get_pending_operations(collection, store, has_non_callable_transactions);
            auto final_id = info.first_id;
            
            if (transactions.empty())
            {
                break;
            }

            // Everything is write ops
            if (has_non_callable_transactions)
            {
                async->submit([
                    info = &info,
                    collection = collection,
                    transactions = std::move(transactions)
                ]()
                {
                    auto col = database::instance->get_collection(collection);
                    auto bulk = col.create_bulk_write();

                    for (auto t : transactions)
                    {
                        bulk.append(std::move(*t->operation));
                    }

                    // Send transactions
                    bulk.execute();

                    // Once we get here, they are all executed, so flag them
                    for (auto t : transactions)
                    {
                        t->done = true;
                        t->pending = false;
                    }
                });
            }
            // Everything is callable ops
            else
            {
                async->submit([
                    this,
                    info = &info,
                    collection = collection,
                    transactions = std::move(transactions)
                ]()
                {
                    auto col = database::instance->get_collection(collection);

                    for (auto t : transactions)
                    {
                        std::move(*t->callable)(col);
                        t->done = true;
                        t->pending = false;
                        --_pending_callables;
                    }
                });
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
    transaction->callable = std::nullopt;
    transaction->done = false;
    transaction->pending = false;

    return slot;
}

uint64_t transaction::push_callable(uint8_t collection, callable_t&& callable)
{
    collection_info& info = _collections[collection];
    uint64_t slot = ++info.current_id;
    transaction_info* transaction = &info.transactions[slot];

    transaction->dependency = std::nullopt;
    transaction->operation = std::nullopt;
    transaction->callable = std::move(callable);
    transaction->done = false;
    transaction->pending = false;
    ++_pending_callables;

    return slot;
}

void transaction::push_dependency(uint8_t collection, uint64_t owner, uint64_t id)
{
    collection_info& info = _collections[collection];
    uint64_t slot = ++info.current_id;
    transaction_info* transaction = &info.transactions[slot];

    transaction->dependency = { .owner = owner, .id = id };
    transaction->operation = std::nullopt;
    transaction->callable = std::nullopt;
    transaction->done = false;
    transaction->pending = false;
}

std::vector<transaction::transaction_info*> transaction::get_pending_operations(uint8_t collection, store_t* store, bool& has_non_callable_transactions)
{
    collection_info& info = _collections[collection];
    std::vector<transaction_info*> transactions;
    std::set<uint64_t> ids;

    has_non_callable_transactions = false;
    bool has_callable_transactions = false;

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

            // Otherwise, the dependency is met, contiune with the next transaction
            continue;
        }

        // Callable transactions can only be executed if there is no pending operation
        if (transaction.callable.has_value())
        {
            if (has_non_callable_transactions)
            {
                break;
            }

            has_callable_transactions = true;
        }
        else
        {
            if (has_callable_transactions)
            {
                break;
            }

            has_non_callable_transactions = true;
        }

        transaction.pending = true;
        transactions.push_back(&transaction);
    }

    info.first_id = id;
    return transactions;
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
