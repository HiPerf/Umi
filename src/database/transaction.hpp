#include "entity/entity.hpp"

#include <boost/circular_buffer.hpp>
#include <mongocxx/model/write.hpp>
#include <tao/tuple/tuple.hpp>

#include <atomic>
#include <optional>
#include <set>


template <typename T, typename B> class static_store;

class transaction : entity<transaction>
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
    using store_t = static_store<transaction, entity<transaction>>;

public:
    void construct();
    void update(store_t* store);

    uint64_t push_operation(uint8_t collection, mongocxx::model::write&& operation);
    void push_dependency(uint8_t collection, uint64_t owner, uint64_t id);

private:
    std::vector<mongocxx::model::write> get_pending_operations(uint8_t collection, store_t* store);
    transaction_info* get_transaction(uint8_t collection, uint64_t id);

private:
    std::unordered_map<uint8_t, collection_info> _collections;
};
