#pragma once

#include <containers/ticket.hpp>
#include <entity/entity.hpp>

#include "maps/offset.hpp"


class transform;


class cell
{
public:
    using offset_t = offset<float, 15>;

    cell(const offset_t& offset);

private:
    offset_t _offset;
    std::vector<typename ticket<entity<transform>>::ptr> _transforms;
};