#ifndef MESH_DATA_H
#define MESH_DATA_H

#include "../Utils/GlobalTypes.h" // Defines Real, Int, amb_dim, dom_dim

#include <vector>
#include <array>

struct MeshData {
    std::vector<std::array<Real, amb_dim>> vertices;
    std::vector<std::array<Int, dom_dim+1>> simplices; // Assuming triangles
};

#endif // MESH_DATA_H