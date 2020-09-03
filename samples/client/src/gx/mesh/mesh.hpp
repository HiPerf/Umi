#pragma once

#include "entity/entity.hpp"
#include "entity/transform.hpp"
#include "gx/shader/program.hpp"
#include "gx/common.hpp"

#include <initializer_list>


class mesh : public entity<mesh>
{
public: 
    void construct(float* vertices, std::size_t vertices_size, int* indices, std::size_t indices_size,
        std::initializer_list<program*>&& progams);

    void scheme_created();

    void sync();

    template <template <typename...> typename S, typename... components>
    constexpr inline void scheme_information(const S<components...>& scheme)
    {
        // Meshes require transforms
        scheme.template require<transform>();
    }

private:
    GLuint _VAO;
    std::vector<program*> _programs;
    ::ticket<entity<transform>>::ptr _transform;
    std::size_t _num_indices;
};



// https://gamedev.stackexchange.com/questions/31308/algorithm-for-creating-spheres
class geometry_provider
{
private:
    static int get_midpoint_index(std::unordered_map<uint64_t, int>& midpoint_indices, std::vector<glm::vec3>& vertices, int i0, int i1) noexcept
    {
        uint64_t key = (((uint64_t)i0) << 32) + i1;
        if (auto it = midpoint_indices.find(key); it != midpoint_indices.end())
        {
            return it->second;
        }

        auto v0 = vertices[i0];
        auto v1 = vertices[i1];
        auto midpoint = (v0 + v1) / 2.0f;

        if (auto it = std::find(vertices.begin(), vertices.end(), midpoint); it != vertices.end())
        {
            return std::distance(vertices.begin(), it);
        }

        int index = vertices.size();
        vertices.push_back(midpoint);
        midpoint_indices.emplace(key, index);
        return index;
    }

public:
    /// <remarks>
    ///      i0
    ///     /  \
    ///    m02-m01
    ///   /  \ /  \
    /// i2---m12---i1
    /// </remarks>
    /// <param name="vectors"></param>
    /// <param name="indices"></param>
    static void subdivide(std::vector<glm::vec3>& vectors, std::vector<int>& indices, bool remove_source_triangles) noexcept
    {
        std::unordered_map<uint64_t, int> midpoint_indices;
        std::vector<int> new_indices;

        if (!remove_source_triangles)
        {
            std::copy(indices.begin(), indices.end(), std::back_inserter(new_indices));
        }

        for (int i = 0; i < indices.size() - 2; i += 3)
        {
            auto i0 = indices[i];
            auto i1 = indices[i + 1];
            auto i2 = indices[i + 2];

            auto m01 = get_midpoint_index(midpoint_indices, vectors, i0, i1);
            auto m12 = get_midpoint_index(midpoint_indices, vectors, i1, i2);
            auto m02 = get_midpoint_index(midpoint_indices, vectors, i2, i0);

            for (int idx : {
                i0, m01, m02,
                i1, m12, m01,
                i2, m02, m12,
                m02, m01, m12
            })
            {
                new_indices.push_back(idx);
            }
        }

        indices.clear();
        std::copy(new_indices.begin(), new_indices.end(), std::back_inserter(indices));
    }

    /// <summary>
    /// create a regular icosahedron (20-sided polyhedron)
    /// </summary>
    /// <param name="primitiveType"></param>
    /// <param name="size"></param>
    /// <param name="vertices"></param>
    /// <param name="indices"></param>
    /// <remarks>
    /// You can create this programmatically instead of using the given vertex 
    /// and index list, but it's kind of a pain and rather pointless beyond a 
    /// learning exercise.
    /// </remarks>

    /// note: icosahedron definition may have come from the OpenGL red book. I don't recall where I found it. 
    static void icosahedron(std::vector<glm::vec3>& vertices, std::vector<int>& indices)
    {
        for (auto idx : {
            0, 4, 1,
            0, 9, 4,
            9, 5, 4,
            4, 5, 8,
            4, 8, 1,
            8, 10, 1,
            8, 3, 10,
            5, 3, 8,
            5, 2, 3,
            2, 7, 3,
            7, 10, 3,
            7, 6, 10,
            7, 11, 6,
            11, 0, 6,
            0, 1, 6,
            6, 1, 10,
            9, 0, 11,
            9, 11, 2,
            9, 2, 5,
            7, 2, 11 })
        {
            indices.push_back(idx + vertices.size());
        }

        auto X = 0.525731112119133606f;
        auto Z = 0.850650808352039932f;

        for (auto vec : {
            glm::vec3(-X, 0.0f, Z),
            glm::vec3(X, 0.0f, Z),
            glm::vec3(-X, 0.0f, -Z),
            glm::vec3(X, 0.0f, -Z),
            glm::vec3(0.0f, Z, X),
            glm::vec3(0.0f, Z, -X),
            glm::vec3(0.0f, -Z, X),
            glm::vec3(0.0f, -Z, -X),
            glm::vec3(Z, X, 0.0f),
            glm::vec3(-Z, X, 0.0f),
            glm::vec3(Z, -X, 0.0f),
            glm::vec3(-Z, -X, 0.0f) })
        {
            vertices.push_back(vec);
        }
    }
};
