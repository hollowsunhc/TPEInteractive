#ifndef VISUALIZATION_ENGINE_H
#define VISUALIZATION_ENGINE_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "../Utils/GlobalTypes.h"  // For Real type if needed for vectors

class SceneObject;
struct ConfigType;

namespace polyscope {
enum class UpDir;
enum class FrontDir;
}  // namespace polyscope

class VisualizationEngine {
  public:
    explicit VisualizationEngine(const ConfigType& config);
    ~VisualizationEngine() = default;

    // --- Object Management ---
    bool RegisterObject(SceneObject& object);
    bool RemoveObjectByName(const std::string& name);
    void RemoveAllObjects();

    // --- Updates ---
    void UpdateObjectTransform(SceneObject& object);
    void UpdateObjectVertices(SceneObject& object);
    void UpdateActiveGizmo(const std::string& oldActiveName, const std::string& newActiveName);

    // --- Vector Visualization ---
    void UpdateVectorQuantity(SceneObject& object, const std::string& quantityName,
                              const std::vector<glm::vec3>& vectors);
    void RemoveVectorQuantity(SceneObject& object, const std::string& quantityName);
    void RemoveAllVectorQuantities(SceneObject& object);

    // --- Obstacle Visuals ---
    void UpdateSingleObstacleVisual(SceneObject& targetObject);
    void ShowObstaclesForAll(const std::vector<std::unique_ptr<SceneObject>>& objects);
    void HideObstaclesForAll(const std::vector<std::unique_ptr<SceneObject>>& objects);

    // --- General ---
    void RequestRedraw();
    void SetCameraView(const glm::vec3& position, const glm::vec3& lookAt, polyscope::UpDir upDir,
                       polyscope::FrontDir frontDir);
    void ResetCamera();

  private:
    const ConfigType& m_config;
};

#endif  // VISUALIZATION_ENGINE_H