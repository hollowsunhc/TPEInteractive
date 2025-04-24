#ifndef HELPERS_H
#define HELPERS_H

#include "GlobalTypes.h"
#include "Repulsor/submodules/Tensors/Tensors.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

#include <vector>
#include <array>
#include <string>

namespace polyscope { class Structure; }

namespace Utils {

// --- Geometry / Math ---
std::vector<std::array<Real, amb_dim>> applyTransform(
    const std::vector<std::array<Real, amb_dim>>& originalVerts,
    const glm::mat4& transform
);

bool matricesAreClose(const glm::mat4& m1, const glm::mat4& m2, float epsilon = 1e-6f);

// --- Tensor Conversions ---
std::vector<std::array<Real, amb_dim>> tensorToVecArray(const Tensors::Tensor2<Real, Int>& tensor);
Tensors::Tensor2<Real, Int> vecArrayToTensor(const std::vector<std::array<Real, amb_dim>>& vecArray);
std::vector<glm::vec3> tensorToGlmVec3(const Tensors::Tensor2<Real, Int>& T);

// --- Visualization Scaling ---
std::vector<glm::vec3> scaleVectorsForVisualization(
    const std::vector<glm::vec3>& rawVectors,
    bool useLog,
    float linearScaleFactor,
    float targetMaxLog
);

// --- Obstacle Combination Data Structure ---
struct CombinedObstacleGeometry {
    std::vector<std::array<Real, 3>> combined_world_vertices;
    std::vector<std::array<Int, 3>>  combined_simplices;
    bool success = false;
};

// --- Physics Step Data Structure ---
struct IterationData {
    Tensors::Tensor2<Real, Int> world_displacement;
    bool updated = false;
};

// --- UI Helpers ---
void HelpMarker(const char* desc);

} // namespace Utils

#endif // HELPERS_H