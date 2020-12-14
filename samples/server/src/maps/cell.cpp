#include "maps/cell.hpp"
#include "entities/transform.hpp"


cell::cell(const offset_t& offset) :
    _offset(offset)
{}

void cell::move_to(cell* other, transform* transform)
{
    auto ticket = transform->ticket();
    _transforms.erase(std::find(_transforms.begin(), _transforms.end(), ticket)); // TODO(gpascualg): Optimize erase with a move
    other->_transforms.push_back(ticket);
}
