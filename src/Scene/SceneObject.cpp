#include "SceneObject.h"

#include <stdexcept>

SceneObject::SceneObject(const SceneObjectDefinition& def) :
    m_id(def.id),
    m_baseName(def.baseName),
    m_isInteractive(def.isInteractive),
    m_isObstacleSource(def.isObstacleSource),
    m_isSimulated(def.isSimulated),
    m_meshDataRef(def.meshData)
{
    m_uniqueName = m_baseName + "_" + std::to_string(m_id);

    if (m_meshDataRef) {
        m_initialVertices = m_meshDataRef->vertices;
    } else {
        throw;
    }
}


const std::vector<std::array<Real, amb_dim>>& SceneObject::GetInitialVertices() const {
    return m_initialVertices;
}


const std::vector<std::array<Int, 3>>& SceneObject::GetSimplices() const {
    if (!m_meshDataRef) {
        static const std::vector<std::array<Int, 3>> empty_simplices;
        return empty_simplices;
    }
    return m_meshDataRef->simplices;
}


void SceneObject::UpdateInitialVertices(const std::vector<std::array<Real, amb_dim>>& local_deltas) {
    if (!m_isSimulated) {
        return;
    }
    if (local_deltas.size() != m_initialVertices.size()) {
        throw std::runtime_error("Vertex count mismatch in UpdateInitialVertices for " + m_uniqueName);
    }

    for (size_t i = 0; i < m_initialVertices.size(); ++i) {
        m_initialVertices[i][0] += local_deltas[i][0];
        m_initialVertices[i][1] += local_deltas[i][1];
        m_initialVertices[i][2] += local_deltas[i][2];
    }
}