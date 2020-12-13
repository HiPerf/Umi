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
        time_point_t timestamp;
        glm::vec3 position;
        glm::vec3 forward;
        float speed;
    };

public:
    transform();

    void construct(const time_point_t& timestamp, const glm::vec3& position, const glm::vec3& forward);
    void push(const time_point_t& timestamp, const glm::vec3& position, const glm::vec3& forward, float speed);

    inline glm::vec3 position(const time_point_t& timestamp) const;

private:
    boost::circular_buffer<physics> _buffer;
};


inline glm::vec3 transform::position(const time_point_t& timestamp) const
{
    const auto& last = _buffer.back();
    return last.position + last.forward * (last.speed * std::chrono::duration_cast<base_time>(timestamp - last.timestamp).count());
}
