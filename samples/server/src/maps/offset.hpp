#pragma once

#include "common/types.hpp"

#include <array>


namespace cell_information
{
    constexpr inline uint8_t num_neighbors = 6;
    constexpr inline uint8_t contiguous_in_side = 3;

    constexpr inline std::array<int8_t, 3> Directions[num_neighbors] = {
        {0, 0, -1},
        {1, 1, -1},
        {2, 1, 0},
        {3, -1, 0},
        {4, 0, 1},
        {5, -1, 1}
    };
};

template <typename CoordType, uint32_t Size>
class offset
{
public:
    using hash_t = uint64_t;

    constexpr static inline uint32_t side_size = Size;
    
    constexpr offset(int32_t x, int32_t y) :
        _x(x),
        _y(y)
    {}

    // Flat-top implementation
    constexpr static offset of(CoordType x, CoordType y)
    {
        const float fx = static_cast<float>(x);
        const float fy = static_cast<float>(y);

        const float q = fx * 2.0f / 3.0f / Size;
        const float r = (-fx / 3.0f + cx::sqrt(3.0f) / 3.0f * fy) / Size;
        const float s = -q - r;

        const int32_t q_int = static_cast<int32_t>(round(static_cast<float>(q)));
        const int32_t r_int = static_cast<int32_t>(round(static_cast<float>(r)));
        const int32_t s_int = static_cast<int32_t>(round(static_cast<float>(s)));

        const float q_diff = cx::abs(static_cast<float>(q_int) - q);
        const float r_diff = cx::abs(static_cast<float>(r_int) - r);
        const float s_diff = cx::abs(static_cast<float>(s_int) - s);

        if (q_diff > r_diff && q_diff > s_diff)
        {
            return offset { -r_int -s_int, r_int };
        }
        else if (r_diff > s_diff)
        {
            return offset { q_int, -q_int -s_int };
        }

        return offset { q_int, r_int };
    }
    
    constexpr offset(const offset&) = default;
    constexpr offset(offset&&) = default;
    constexpr offset& operator=(const offset&) = default;
    constexpr offset& operator=(offset&&) = default;

    constexpr bool operator==(const offset& other) const
    {
        return hash() == other.hash();
    }

    constexpr bool operator!=(const offset& other) const
    {
        return !(*this == other);
    }

    inline constexpr int32_t x() const { return _x; }
    inline constexpr int32_t y() const { return _y; }
    inline constexpr int32_t s() const { return -x() - y(); }

    constexpr hash_t hash() const
    {
        return hash(x(), y());
    }

    constexpr static hash_t hash(int32_t x, int32_t y)
    {
        return (as_u64(y) << 32) | as_u64(x);
    }

    constexpr int32_t distance(const offset& offset) const
    {
        const int32_t dq = cx::abs(x() - offset.x());
        const int32_t dr = cx::abs(y() - offset.y());
        const int32_t ds = cx::abs(s() - offset.s());
        return cx::max(cx::max(dq, dr), ds);
    }

    constexpr glm::vec2 center() const
    {
        return { 
            Size * 3.0f / 2.0f * static_cast<float>(x()), 
            Size * cx::sqrt(3.0f) * (static_cast<float>(y()) + static_cast<float>(x()) / 2.0f) 
        };
    }

    constexpr std::tuple<float, float, float, float> bbox() const
    {
        constexpr float H = cx::sqrt(3.0f) * Size / 2.0f;
        return { 
            Size * 3.0f / 2.0f * static_cast<float>(x()) - Size, // x0
            Size * cx::sqrt(3.0f) * (static_cast<float>(y()) + static_cast<float>(x()) / 2.0f) - H, // y0
            Size * 3.0f / 2.0f * static_cast<float>(x()) + Size, // x1
            Size * cx::sqrt(3.0f) * (static_cast<float>(y()) + static_cast<float>(x()) / 2.0f) + H, // y1
        };
    }

    constexpr std::tuple<
        glm::vec2, glm::vec2, glm::vec2,
        glm::vec2, glm::vec2, glm::vec2> vertices() const
    {
        constexpr float H = cx::sqrt(3.0f) * Size / 2.0f;
        constexpr float W = Size / 2.0f;
        return {
            { -Size, 0 },
            { -W, H },
            { W, H },
            { 0, Size },
            { W, -H },
            { -W, -H }
        };
    }

private:
    int32_t _x;
    int32_t _y;
};

template <typename C, uint32_t S>
constexpr inline std::array<offset<C, S>, cell_information::num_neighbors> neighbours_of(const offset<C, S>& offset)
{
    return {
        offset<C, S>(offset.x() + 0, offset.y() - 1),
        offset<C, S>(offset.x() + 1, offset.y() - 1),
        offset<C, S>(offset.x() + 1, offset.y() + 0),
        offset<C, S>(offset.x() - 1, offset.y() + 0),
        offset<C, S>(offset.x() + 0, offset.y() + 1),
        offset<C, S>(offset.x() - 1, offset.y() + 1),
    };
}

template <typename C, uint32_t S>
constexpr inline std::array<offset<C, S>, cell_information::contiguous_in_side> new_offsets_in_direction(const offset<C, S>& offset, int32_t dx, int32_t dy)
{
    return {
        offset<C, S>(offset.x() + dx, offset.y() + dy),
        offset<C, S>(offset.x() + dx + dy, offset.y() - dx),
        offset<C, S>(offset.x() - dy, offset.y() + dx + dy),
    };
}

template <typename C, uint32_t S>
constexpr bool operator<(const offset<C, S>& lhs, const offset<C, S>& rhs)
{
    return (lhs.x() < rhs.x()) || ((lhs.x() == rhs.x()) && (lhs.y() < rhs.y()));
}
