#ifndef FCC_LATTICE_SPHERES_H
#define FCC_LATTICE_SPHERES_H

#include "../Utils/GlobalTypes.h"
#include "EmbeddedMeshData.h"

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

inline std::vector<std::vector<std::array<Real, 3>>>
createFCCLatticeSpheres(const std::array<Real, 3>& region_min,
                        const std::array<Real, 3>& region_max,
                        double sphereRadius = 1.0,
                        double centerSeparation = 2.0,
                        double boundaryMargin = 0.0)
{
    std::vector<std::vector<std::array<Real, 3>>> spheres;

    const Real s = std::sqrt(2.0);
    const Real scale = centerSeparation / 2.0;
    const std::array<Real, 3> a1 = {0.0, s * scale, s * scale};
    const std::array<Real, 3> a2 = {s * scale, 0.0, s * scale};
    const std::array<Real, 3> a3 = {s * scale, s * scale, 0.0};

    Real region_extent = std::max({region_max[0] - region_min[0],
                                   region_max[1] - region_min[1],
                                   region_max[2] - region_min[2]});
    int N = static_cast<int>(std::ceil(region_extent / (s * scale))) + 1;

    const size_t template_vertex_count = EmbeddedData::two_spheres_vertex_count;
    const auto& template_vertex_coords = EmbeddedData::two_spheres_vertex_coordinates;

    for (int i = -N; i <= N; ++i) {
        for (int j = -N; j <= N; ++j) {
            for (int k = -N; k <= N; ++k) {
                std::array<Real, 3> center = {
                    i * a1[0] + j * a2[0] + k * a3[0],
                    i * a1[1] + j * a2[1] + k * a3[1],
                    i * a1[2] + j * a2[2] + k * a3[2]
                };

                bool fits = true;
                for (int d = 0; d < 3; ++d) {
                    if (center[d] - sphereRadius < region_min[d] + boundaryMargin ||
                        center[d] + sphereRadius > region_max[d] - boundaryMargin) {
                        fits = false; break;
                    }
                }
                if (!fits) continue;

                std::vector<std::array<Real, 3>> sphere_vertices_vec;
                sphere_vertices_vec.reserve(template_vertex_count);

                for (size_t v = 0; v < template_vertex_count; ++v) {
                    Real x = template_vertex_coords[v][0] * sphereRadius + center[0];
                    Real y = template_vertex_coords[v][1] * sphereRadius + center[1];
                    // Assuming template is centered at (0,0,1) originally
                    Real z = (template_vertex_coords[v][2] - 1.0) * sphereRadius + center[2];
                    sphere_vertices_vec.push_back({x, y, z});
                }
                spheres.push_back(std::move(sphere_vertices_vec));
            }
        }
    }
    return spheres;
}

#endif // FCC_LATTICE_SPHERES_H