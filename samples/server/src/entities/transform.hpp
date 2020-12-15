#pragma once

#include "common/types.hpp"
#include "common/definitions.hpp"

#include <entity/entity.hpp>

#include <boost/circular_buffer.hpp>
#include <glm/glm.hpp>


class map;
class cell;
class region;

struct moving_flag_t {};

class transform : public entity<transform>
{
    struct physics
    {
        time_point_t timestamp;
        glm::vec3 position;
        glm::vec3 forward;
        float speed;
    };

public:
    transform();

    void construct(map* map, region* region, cell* cell);
    void update(const base_time& diff, map* map);

    void push(const time_point_t& timestamp, const glm::vec3& position, const glm::vec3& forward, float speed);

    inline region* current_region() const;
    inline cell* current_cell() const;

    inline glm::vec3 position(const time_point_t& timestamp) const;
    inline bool is_moving() const;

private:
    boost::circular_buffer<physics> _buffer;
    region* _current_region;
    cell* _current_cell;
    bool _is_moving;
};


inline region* transform::current_region() const
{
    return _current_region;
}

inline cell* transform::current_cell() const
{
    return _current_cell;
}

inline glm::vec3 transform::position(const time_point_t& timestamp) const
{
    const auto& last = _buffer.back();
    return last.position + last.forward * (last.speed * std::chrono::duration_cast<base_time>(timestamp - last.timestamp).count());
}

inline bool transform::is_moving() const
{
    return _is_moving;
}
