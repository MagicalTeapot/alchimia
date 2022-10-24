#include "world.hpp"
#include "pixel.hpp"
#include "update_functions.hpp"
#include "utility.hpp"

#include <cassert>
#include <algorithm>
#include <ranges>

namespace sand {
namespace {

static const auto default_pixel = pixel::air();

auto get_pos(glm::vec2 pos) -> std::size_t
{
    return pos.x + world_size * pos.y;
}

auto get_chunk_pos(glm::vec2 chunk) -> std::size_t
{
    return chunk.x + num_chunks * chunk.y;
}

}

world::world()
{
    d_pixels.fill(pixel::air());
}

auto world::valid(glm::ivec2 pos) const -> bool
{
    return 0 <= pos.x && pos.x < world_size && 0 <= pos.y && pos.y < world_size;
}

auto world::simulate() -> void
{
    for (auto& chunk : d_chunks) {
        chunk.should_step = chunk.should_step_next;
        chunk.should_step_next = false;
    }
    
    const auto inner = [&] (glm::ivec2 pos) {
        if (is_chunk_awake(pos) && !at(pos).flags[is_updated]) {
            update_pixel(*this, pos);
        }
    };

    for (std::uint32_t y = world_size; y != 0; --y) {
        if (coin_flip()) {
            for (std::uint32_t x = 0; x != world_size; ++x) {
                inner({x, y - 1});
            }
        }
        else {
            for (std::uint32_t x = world_size; x != 0; --x) {
                inner({x - 1, y - 1});
            }
        }
    }

    std::ranges::for_each(d_pixels, [](auto& p) { p.flags[is_updated] = false; });
}

auto world::set(glm::ivec2 pos, const pixel& pixel) -> void
{
    assert(valid(pos));
    wake_chunk_with_pixel(pos);
    d_pixels[get_pos(pos)] = pixel;
}

auto world::fill(const pixel& p) -> void
{
    d_pixels.fill(p);
}

auto world::at(glm::ivec2 pos) const -> const pixel&
{
    if (!valid(pos)) {
        return default_pixel;
    }
    return d_pixels[get_pos(pos)];
}

auto world::at(glm::ivec2 pos) -> pixel&
{
    assert(valid(pos));
    return d_pixels[get_pos(pos)];
}

auto world::swap(glm::ivec2 lhs, glm::ivec2 rhs) -> glm::ivec2
{
    wake_chunk_with_pixel(lhs);
    wake_chunk_with_pixel(rhs);
    std::swap(at(lhs), at(rhs));
    return rhs;
}

auto world::wake_chunk_with_pixel(glm::ivec2 pixel) -> void
{
    const auto chunk = pixel / static_cast<int>(chunk_size);
    d_chunks[get_chunk_pos(chunk)].should_step_next = true;

    // Wake right
    if (pixel.x != world_size - 1 && (pixel.x + 1) % chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{1, 0};
        if (valid(neighbour))
            d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake left
    if (pixel.x != 0 && (pixel.x - 1) % chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{1, 0};
        if (valid(neighbour))
            d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake down
    if (pixel.y != world_size - 1 && (pixel.y + 1) % chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{0, 1};
        if (valid(neighbour))
            d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake up
    if (pixel.y != 0 && (pixel.y - 1) % chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{0, 1};
        if (valid(neighbour))
            d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }
}

auto world::wake_all_chunks() -> void
{
    for (auto& chunk : d_chunks) {
        chunk.should_step_next = true;
    }
}

auto world::num_awake_chunks() const -> std::size_t
{
    auto count = std::size_t{0};
    for (const auto& chunk : d_chunks) {
        count += static_cast<std::size_t>(chunk.should_step);
    }
    return count;
}

auto world::is_chunk_awake(glm::ivec2 pixel) const -> bool
{
    const auto chunk = pixel / static_cast<int>(chunk_size);
    return d_chunks[get_chunk_pos(chunk)].should_step;
}

}