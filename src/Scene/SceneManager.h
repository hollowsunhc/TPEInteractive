#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "../Data/SceneDefinition.h"
#include "../Engine/RepulsorEngine.h"
#include "../Utils/Helpers.h"

#include <vector>
#include <string>
#include <memory>
#include <map>

class RepulsorEngine;
class VisualizationEngine;
struct ConfigType;

class SceneManager {
public:
    SceneManager(RepulsorEngine& repulsorEngine, VisualizationEngine& vizEngine, const ConfigType& config);
    ~SceneManager() = default;

    bool LoadScene(const SceneDefinition& sceneDef);
    void UnloadScene();

    // Updates triggered by user interaction (e.g., gizmo)
    void UpdateObjectTransform(int objectId, const glm::mat4& newTransform);
    void UpdateEngineParametersForAllObjects();

    // Updates triggered by physics step
    void ApplyPhysicsStep(int iterations);

    // Getters for UI or other components
    const std::vector<std::unique_ptr<SceneObject>>& GetObjects() const;
    SceneObject* GetObjectById(int id); // Returns nullptr if not found
    SceneObject* GetActiveObject();
    int GetActiveObjectId() const { return m_activeObjectId; }
    bool SetActiveObjectId(int id);
    //const SceneDefinition* GetCurrentSceneDefinition() const;

private:
    // Core simulation logic separated for clarity
    bool CalculateAndApplyPhysicsUpdates(std::map<int, Utils::IterationData>& results);
    void SynchronizeObjectState(int objectId, const glm::mat4& newTransform); // Internal gizmo update handler

    // --- Obstacle Logic ---
    void UpdateObstaclesForAllObjects(); // Called after any state change
    Utils::CombinedObstacleGeometry CalculateCombinedObstacleGeometryForObject(int targetObjectId);
    void UpdateRepulsorObstacleForObject(SceneObject& targetObject, const Utils::CombinedObstacleGeometry& obsGeo);

    RepulsorEngine& m_repulsorEngine;
    VisualizationEngine& m_vizEngine;
    const ConfigType& m_config;

    std::unique_ptr<SceneDefinition> m_currentSceneDef;
    std::vector<std::unique_ptr<SceneObject>> m_objects;
    int m_activeObjectId = -1;
};

#endif // SCENE_MANAGER_H