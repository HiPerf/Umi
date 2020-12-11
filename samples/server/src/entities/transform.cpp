#include "entities/transform.hpp"


transform::transform() :
    entity<transform>(),
    _buffer(16)
{}

void transform::construct(const base_time& timestamp, const glm::vec3& position, const glm::vec3& forward)
{
    _buffer.clear();
    push(timestamp, position, forward, 0.0f);
}

void transform::push(const base_time& timestamp, const glm::vec3& position, const glm::vec3& forward, float speed)
{
    _buffer.push_back({
        .timestamp = timestamp,
        .position = position,
        .forward = forward,
        .speed = speed
    });
}
