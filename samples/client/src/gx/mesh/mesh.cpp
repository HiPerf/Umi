#include "gx/mesh/mesh.hpp"
#include "containers/dictionary.hpp"


void mesh::construct(float* vertices, std::size_t vertices_size, int* indices, std::size_t indices_size,
    std::initializer_list<program*>&& progams)
{
    unsigned int VBO, EBO;
    glGenVertexArrays(1, &_VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertices_size, (const void*)vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indices_size, (const void*)indices, GL_STATIC_DRAW);

    _programs = std::move(progams);
    for (auto progam : _programs)
    {
        progam->use_attributes();
    }

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); 

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

        glBindVertexArray(_VAO);
        glDrawElements(GL_TRIANGLES, _num_indices, GL_UNSIGNED_INT, 0);
    }
}
