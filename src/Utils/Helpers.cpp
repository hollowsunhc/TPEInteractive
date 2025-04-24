#include "Helpers.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <stdexcept>
#include <limits>
#include <cmath>


namespace Utils {

// --- Geometry / Math ---
std::vector<std::array<Real, amb_dim>> applyTransform(
    const std::vector<std::array<Real, amb_dim>>& originalVerts,
    const glm::mat4& transform)
{
    std::vector<std::array<Real, amb_dim>> transformedVerts;
    transformedVerts.reserve(originalVerts.size());
    for (const auto& v_orig_arr : originalVerts) {
        glm::vec4 v_orig = { (float)v_orig_arr[0], (float)v_orig_arr[1], (float)v_orig_arr[2], 1.0f };
        glm::vec4 v_transformed = transform * v_orig;
        transformedVerts.push_back({ (Real)v_transformed.x, (Real)v_transformed.y, (Real)v_transformed.z });
    }
    return transformedVerts;
}


bool matricesAreClose(const glm::mat4& m1, const glm::mat4& m2, float epsilon) {
    for(int i=0; i<4; ++i) {
        for(int j=0; j<4; ++j) {
            if (std::abs(m1[i][j] - m2[i][j]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}


// --- Tensor Conversions ---
std::vector<std::array<Real, amb_dim>> tensorToVecArray(const Tensors::Tensor2<Real, Int>& tensor) {
    int nRows = tensor.Dimension(0);
    int nCols = tensor.Dimension(1);
    if (nCols != amb_dim) {
        throw std::runtime_error("tensorToVecArray: Tensor column count " + std::to_string(nCols) +
                                 " does not match ambient dimension " + std::to_string(amb_dim));
    }
    std::vector<std::array<Real, amb_dim>> vec(nRows);
    for (int i = 0; i < nRows; ++i) {
        for(int j=0; j<amb_dim; ++j) {
            vec[i][j] = tensor(i, j);
        }
    }
    return vec;
}


Tensors::Tensor2<Real, Int> vecArrayToTensor(const std::vector<std::array<Real, amb_dim>>& vecArray) {
    Int nRows = static_cast<Int>(vecArray.size());
    Tensors::Tensor2<Real, Int> tensor(nRows, amb_dim);
    for(Int i = 0; i < nRows; ++i) {
        for(Int j = 0; j < amb_dim; ++j) {
            tensor(i, j) = vecArray[i][j];
        }
    }
    return tensor;
}


std::vector<glm::vec3> tensorToGlmVec3(const Tensors::Tensor2<Real, Int>& T) {
    int n = T.Dimension(0);
    int m = T.Dimension(1);
    if (m != 3) {
        throw std::runtime_error("tensorToGlmVec3: Expected tensor to have 3 columns, got " + std::to_string(m));
    }
    std::vector<glm::vec3> result;
    result.reserve(n);
    for (int i = 0; i < n; i++) {
        result.emplace_back(static_cast<float>(T(i, 0)), static_cast<float>(T(i, 1)), static_cast<float>(T(i, 2)));
    }
    return result;
}


// --- Visualization Scaling ---
std::vector<glm::vec3> scaleVectorsForVisualization(
    const std::vector<glm::vec3>& rawVectors,
    bool useLog,
    float linearScaleFactor,
    float targetMaxLog)
{
    std::vector<glm::vec3> scaledVectors;
    scaledVectors.reserve(rawVectors.size());

    if (!useLog) {
        // --- Linear Scaling ---
        for (const auto& v : rawVectors) {
            scaledVectors.push_back(v * linearScaleFactor);
        }

    } else {
        // --- Logarithmic Scaling ---
        float minMag = std::numeric_limits<float>::max();
        float maxMag = 0.0f;
        for (const auto& v : rawVectors) {
            float len = glm::length(v);
            if (len > 1e-12f) { minMag = std::min(minMag, len); maxMag = std::max(maxMag, len); }
        }
        if (minMag == std::numeric_limits<float>::max() || maxMag <= minMag || minMag <= 0.0f || (log(maxMag) - log(minMag)) < 1e-6f ) {
            for (const auto& v : rawVectors) { scaledVectors.push_back(v * linearScaleFactor); }
            return scaledVectors;
        }
        float logMin = std::log(minMag); float logMax = std::log(maxMag); float logRange = logMax - logMin;
        for (const auto& v : rawVectors) {
            float len = glm::length(v);
            if (len <= 1e-12f) { scaledVectors.emplace_back(0.0f, 0.0f, 0.0f); }
            else {
                float safeLength = std::max(len, minMag);
                float normalizedLog = (std::log(safeLength) - logMin) / logRange;
                float visualScale = normalizedLog * targetMaxLog;
                scaledVectors.push_back(glm::normalize(v) * visualScale);
            }
        }
    }
    return scaledVectors;
}


// --- UI Helpers ---
void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

} // namespace Utils