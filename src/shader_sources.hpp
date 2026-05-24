#pragma once

#include <array>
#include <cstdint>

namespace vkBasalt
{
    constexpr static auto cas_frag = std::to_array<std::uint32_t>({
#include <cas.frag.spv>
    });

    constexpr static auto deband_frag = std::to_array<std::uint32_t>({
#include <deband.frag.spv>
    });

    constexpr static auto dls_frag = std::to_array<std::uint32_t>({
#include <dls.frag.spv>
    });

    constexpr static auto full_screen_triangle_vert = std::to_array<std::uint32_t>({
#include <full_screen_triangle.vert.spv>
    });

    constexpr static auto fxaa_frag = std::to_array<std::uint32_t>({
#include <fxaa.frag.spv>
    });

    constexpr static auto lut_frag = std::to_array<std::uint32_t>({
#include <lut.frag.spv>
    });

    constexpr static auto smaa_blend_frag = std::to_array<std::uint32_t>({
#include <smaa_blend.frag.spv>
    });

    constexpr static auto smaa_blend_vert = std::to_array<std::uint32_t>({
#include <smaa_blend.vert.spv>
    });

    constexpr static auto smaa_edge_color_frag = std::to_array<std::uint32_t>({
#include <smaa_edge_color.frag.spv>
    });

    constexpr static auto smaa_edge_luma_frag = std::to_array<std::uint32_t>({
#include <smaa_edge_luma.frag.spv>
    });

    constexpr static auto smaa_edge_vert = std::to_array<std::uint32_t>({
#include <smaa_edge.vert.spv>
    });

    constexpr static auto smaa_neighbor_frag = std::to_array<std::uint32_t>({
#include <smaa_neighbor.frag.spv>
    });

    constexpr static auto smaa_neighbor_vert = std::to_array<std::uint32_t>({
#include <smaa_neighbor.vert.spv>
    });
} // namespace vkBasalt
