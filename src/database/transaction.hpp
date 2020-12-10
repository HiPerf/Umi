#include "entity/entity.hpp"

#include <boost/circular_buffer.hpp>
#include <mongocxx/model/write.hpp>
#include <tao/tuple/tuple.hpp>

#include <atomic>
#include <optional>
#include <set>


template <typename T, typename B> class static_store;
class async_executor_base;

class transaction : public entity<transaction>
{
    struct transaction_info
    {
        struct dependency
        {
            uint64_t owner;
            uint64_t id;
        };

        std::optional<struct dependency> dependency;
        std::optional<mongocxx::model::write> operation;
        bool pending;
        bool done;
    };

    struct collection_info
    {
        collection_info();

        uint64_t first_id;
        std::atomic<uint64_t> current_id;
        std::unordered_map<uint64_t, transaction_info> transactions;
    };

public:
    using store_t = store<transaction, entity<transaction>>;

public:
    void construct(uint64_t execute_every);
    void update(uint64_t diff, store_t* store, async_executor_base* async);

    uint64_t push_operation(uint8_t collection, mongocxx::model::write&& operation);
    void push_dependency(uint8_t collection, uint64_t owner, uint64_t id);

    inline void flag_deletion();
    inline void unflag_deletion();

private:
    std::vector<mongocxx::model::write> get_pending_operations(uint8_t collection, store_t* store);
    transaction_info* get_transaction(uint8_t collection, uint64_t id);

private:
    std::unordered_map<uint8_t, collection_info> _collections;
    uint64_t _execute_every;
    uint64_t _since_last_execution;
    bool _flagged;
};

inline void transaction::flag_deletion()
{
    _flagged = true;
}

inline void transaction::unflag_deletion()
{
    _flagged = false;
}
