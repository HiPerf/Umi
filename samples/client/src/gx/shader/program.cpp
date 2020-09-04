#include "gx/common.hpp"
#include "gx/shader/program.hpp"
#include "io/memmap.hpp"


program::program()
{}

GLuint program::attach(GLenum type, std::filesystem::path path)
{
    GLuint shader = compile(type, path);
    if (shader != 0)
    {
        /* attach the shader to the program */
        glAttachShader(_program, shader);

        /* delete the shader - it won't actually be
        * destroyed until the program that it's attached
        * to has been destroyed */
        glDeleteShader(shader);
    }

    return shader;
}

GLuint program::compile(GLenum type, const std::filesystem::path& path)
{
    auto file_mapping = map_file(path.string().c_str());
    if (!file_mapping)
    {
        return 0;
    }

    GLuint shader = GL_SAFE_EX(tao::tuple(0), glCreateShader, type);
    GL_SAFE(glShaderSource, shader, 1, (const char **)&file_mapping->addr, &file_mapping->length);
    GL_SAFE(glCompileShader, shader);
    
    unmap_file(*file_mapping);

    /* Make sure the compilation was successful */
    GLint result;
    GL_SAFE(glGetShaderiv, shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        /* get the shader info log */
        GL_SAFE(glGetShaderiv, shader, GL_INFO_LOG_LENGTH, &file_mapping->length);
        char* log = new char[file_mapping->length];
        GL_SAFE(glGetShaderInfoLog, shader, file_mapping->length, &result, log);

        /* print an error message and the info log */
        //LOGD("Unable to compile %s: %s", path.c_str(), log);
        delete[] log;

        GL_SAFE(glDeleteShader, shader);
        return 0;
    }

    return shader;
}

bool program::bind_attribute(const char* attrib, GLuint index)
{
    _attribs.emplace(attrib, index);
    GL_SAFE(glBindAttribLocation, _program, index, attrib);
    return true;
}

#include <iostream>

bool program::link()
{
    GLint result;

    /* link the program and make sure that there were no errors */
    GL_SAFE(glLinkProgram, _program);
    GL_SAFE(glGetProgramiv, _program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE)
    {
        GLint length;

        /* get the program info log */
        GL_SAFE(glGetProgramiv, _program, GL_INFO_LOG_LENGTH, &length);
        char* log = new char[length];
        GL_SAFE(glGetProgramInfoLog, _program, length, &result, log);

        /* print an error message and the info log */
        //LOGD("Program linking failed: %s", log);
        delete[] log;

        /* delete the program */
        GL_SAFE(glDeleteProgram, _program);
        _program = 0;
    }

    // Before linking, bind global matrices
    auto matrices_loc = GL_SAFE_EX(tao::tuple(GL_INVALID_INDEX), glGetUniformBlockIndex, _program, "GlobalMatrices");
	GL_SAFE(glUniformBlockBinding, _program, matrices_loc, GlobalMatricesIndex);

    return _program != 0;
}

void program::attribute_pointer(const char* attrib, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)
{
    auto pos = attribute_location(attrib);
    _stored_attributes.push_back(attribute {
        .pos = pos,
        .size = size,
        .type = type,
        .normalized = normalized,
        .stride = stride,
        .ptr = ptr
    });
}


void program::use_attributes()
{
    for (auto& attr : _stored_attributes)
    {
        GL_SAFE(glEnableVertexAttribArray, attr.pos);
        GL_SAFE(glVertexAttribPointer, attr.pos, attr.size, attr.type, attr.normalized, attr.stride, attr.ptr);
    }
}
