#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "entity/entity.hpp"


class camera : public entity<camera>
{
public:
    void construct(const glm::vec3& scene_dimensions);

    inline glm::mat4 mvp() const;
    inline glm::vec3 obs() const;
    inline glm::vec3 up() const;
    inline glm::vec3 forward() const;
    inline glm::mat4 model() const;
    inline glm::mat4 proj() const;

    void look_towards(glm::vec3 dir);
    void look_to(glm::vec3 dir);
    void move(glm::vec3 amount);

private:
    void calculate_view();
    void calculate_proj();
    void schedule();

private:
    bool _scheduled;
    glm::mat4 _model;
    glm::mat4 _view;
    glm::mat4 _proj;
    glm::mat4 _mvp;

    glm::vec3 _obs;
    glm::vec3 _dir;
    glm::vec3 _right;
    glm::vec3 _vup;

    GLuint _matrices_ubo;
};


inline glm::mat4 camera::mvp() const
{
    return _mvp;
}

inline glm::vec3 camera::obs() const
{
    return _obs;
}

inline glm::vec3 camera::up() const
{
    return _vup;
}

inline glm::vec3 camera::forward() const
{
    return -_dir;
}

inline glm::mat4 camera::model() const
{
    return _model;
}

inline glm::mat4 camera::proj() const
{
    return _proj;
}



