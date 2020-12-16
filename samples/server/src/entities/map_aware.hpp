#pragma once

#include <entity/entity.hpp>


class cell;
class region;

class map_aware : public entity<map_aware>
{
public:
    void construct(int64_t db_id);
    void destroy();

    inline int64_t db_id() const;

private:
    int64_t _db_id;
};


inline int64_t map_aware::db_id() const
{
    return _db_id;
}
