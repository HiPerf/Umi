#include "gx/common.hpp"
#include "gx/camera/camera.hpp"
#include "updater/executor.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/string_cast.hpp>


void camera::construct(const glm::vec3& scene_dimensions)
{
    glGenBuffers(1, &_matrices_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, _matrices_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, GlobalMatricesIndex, _matrices_ubo, 0, sizeof(glm::mat4));
    
    // Global scaling
    _model = glm::scale(glm::mat4(1.0), 2.0f / scene_dimensions);

    // Camera position
    _obs = _model * glm::vec4(0, 349600000, 0, 1);
    _dir = glm::normalize(_obs - glm::vec3(0, 0, 0)); // Target origin

    // World up is Z
    _right = glm::normalize(glm::cross(glm::vec3(1, 0, 0), _dir));
    _vup = glm::cross(_dir, _right);

    calculate_view();
    calculate_proj();
}

void camera::look_towards(glm::vec3 dir)
{
    auto diff = dir - _dir;
    _vup = glm::normalize(_vup + diff * _vup); 

    _dir = dir;
    calculate_view();
}

void camera::look_to(glm::vec3 dir)
{
    _dir = -dir;
    //_right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), _dir));
    //_vup = glm::cross(_dir, _right);
    calculate_view();
}

void camera::move(glm::vec3 amount)
{
    _obs += amount;
    calculate_view();
}

void camera::calculate_view()
{
    _view = glm::lookAt(_obs, _obs - _dir, _vup);
    schedule();
}

void camera::calculate_proj()
{
    _proj = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 149600000.0f * 100.0f);
    schedule();
}

void camera::schedule()
{
    if (!_scheduled)
    {
        _scheduled = true;
        
        executor::last->schedule([this]() {
            _mvp = _proj * _view * _model;

            glBindBuffer(GL_UNIFORM_BUFFER, _matrices_ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(_mvp));
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            _scheduled = false;
        });
    }
}
