#include "gl_safe.hpp"
#include "gx/mesh/mesh.hpp"
#include "containers/dictionary.hpp"


void mesh::construct(float* vertices, std::size_t vertices_size, int* indices, std::size_t indices_size,
    std::initializer_list<program*>&& progams)
{
    unsigned int VBO, EBO;
    GL_SAFE(glGenVertexArrays, 1, &_VAO);
    GL_SAFE(glBindVertexArray, _VAO);
    GL_SAFE(glGenBuffers, 1, &VBO);
    GL_SAFE(glGenBuffers, 1, &EBO);

    GL_SAFE(glBindBuffer, GL_ARRAY_BUFFER, VBO);
    GL_SAFE(glBufferData, GL_ARRAY_BUFFER, (GLsizeiptr)vertices_size, (const void*)vertices, GL_STATIC_DRAW);

    GL_SAFE(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, EBO);
    GL_SAFE(glBufferData, GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indices_size, (const void*)indices, GL_STATIC_DRAW);

    _programs = std::move(progams);
    for (auto progam : _programs)
    {
        progam->use_attributes();
    }

    GL_SAFE(glEnableVertexAttribArray, 0);
    GL_SAFE(glBindBuffer, GL_ARRAY_BUFFER, 0);
    GL_SAFE(glBindVertexArray, 0);

    _num_indices = indices_size / sizeof(int);
}

void mesh::scheme_created()
{
    _transform = store<entity<transform>>::get(id());
}

void mesh::sync()
{
    for (auto program : _programs)
    {
        program->bind();
        program->use_uniforms(this);

        GL_SAFE(glBindVertexArray, _VAO);
        GL_SAFE(glDrawElements, GL_TRIANGLES, _num_indices, GL_UNSIGNED_INT, (const void*)0);
        GL_SAFE(glBindVertexArray, 0);
    }
}
