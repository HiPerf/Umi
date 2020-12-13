#pragma once

#include <entity/entity.hpp>


class cell;
class region;

class map_aware : public entity<map_aware>
{
public:
    void construct();

    inline cell* current_cell() const;
    inline region* current_region() const;

private:
    cell* _current_cell;
    region* _current_region;
};

inline cell* map_aware::current_cell() const
{
    return _current_cell;
}

inline region* map_aware::current_region() const
{
    return _current_region;
}
