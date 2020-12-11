#pragma once

#include "common/types.hpp"
#include "common/definitions.hpp"

#include <entity/entity.hpp>

#include <boost/circular_buffer.hpp>
#include <glm/glm.hpp>


class transform : public entity<transform>
{
    struct physics
    {
        base_time timestamp;
        glm::vec3 position;
        glm::vec3 forward;
        float speed;
    };

public:
    transform();

    void construct(const base_time& timestamp, const glm::vec3& position, const glm::vec3& forward);
    void push(const base_time& timestamp, const glm::vec3& position, const glm::vec3& forward, float speed);

    inline glm::vec3 position(const base_time& timestamp) const;

private:
    boost::circular_buffer<physics> _buffer;
};


inline glm::vec3 transform::position(const base_time& timestamp) const
{
    const auto& last = _buffer.back();
    return last.position + last.forward * (last.speed * (timestamp - last.timestamp).count());
}
