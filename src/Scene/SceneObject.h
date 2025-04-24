#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include "../Data/SceneDefinition.h"
#include "../Utils/GlobalTypes.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <array>
#include <memory>


class SceneObject {
public:
    explicit SceneObject(const SceneObjectDefinition& def);
    ~SceneObject() = default;

    // --- Getters for properties defined at load time ---
    int GetId() const { return m_id; }
    const std::string& GetBaseName() const { return m_baseName; }
    const std::string& GetUniqueName() const { return m_uniqueName; }
    bool IsInteractive() const { return m_isInteractive; }
    bool IsObstacleSource() const { return m_isObstacleSource; }
    bool IsSimulated() const { return m_isSimulated; }
    const std::vector<std::array<Real, amb_dim>>& GetInitialVertices() const;
    const std::vector<std::array<Int, 3>>& GetSimplices() const;

    // --- Getters/Setters for runtime state ---
    const glm::mat4& GetCurrentTransform() const { return m_currentTransform; }
    void SetCurrentTransform(const glm::mat4& transform) { m_currentTransform = transform; }

    Mesh_T* GetRepulsorMesh() const { return m_repulsorMesh.get(); }
    void SetRepulsorMesh(std::unique_ptr<Mesh_T> mesh) { m_repulsorMesh = std::move(mesh); }

    // Method to update base vertices (used by physics step)
    void UpdateInitialVertices(const std::vector<std::array<Real, amb_dim>>& local_deltas);

private:
    // --- Static properties from definition ---
    int m_id;
    std::string m_baseName;
    std::string m_uniqueName;
    bool m_isInteractive;
    bool m_isObstacleSource;
    bool m_isSimulated;
    std::shared_ptr<MeshData> m_meshDataRef;

    // --- Runtime state ---
    glm::mat4 m_currentTransform = glm::mat4(1.0f);
    std::vector<std::array<Real, amb_dim>> m_initialVertices; // THIS GETS MODIFIED BY PHYSICS
    std::unique_ptr<Mesh_T> m_repulsorMesh = nullptr;
};

#endif // SCENE_OBJECT_H