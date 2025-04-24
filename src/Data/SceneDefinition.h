#ifndef SCENE_DEFINITION_H
#define SCENE_DEFINITION_H

#include "MeshData.h"
#include "../Utils/GlobalTypes.h"

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <memory>

namespace polyscope {
    enum class UpDir;
    enum class FrontDir;
}


enum class ExampleId {
    FCC_4,
    TWO_SPHERES
    // Add other examples here
};


// Defines the static properties of an object in a scene
struct SceneObjectDefinition {
    int id = -1;
    std::string baseName;
    std::shared_ptr<MeshData> meshData;
    bool isInteractive = false;
    bool isObstacleSource = false;
    bool isSimulated = false;
    std::vector<int> obstacleDefinitionIds;

    SceneObjectDefinition() = default;

    SceneObjectDefinition(int _id, std::string _name, std::shared_ptr<MeshData> _mesh,
                          bool _interactive, bool _obstacleSource, bool _simulated,
                          std::vector<int> _obstacleIds)
        : id(_id), baseName(std::move(_name)), meshData(std::move(_mesh)),
          isInteractive(_interactive), isObstacleSource(_obstacleSource), isSimulated(_simulated),
          obstacleDefinitionIds(std::move(_obstacleIds)) {}
};


// Defines a complete scene setup
struct SceneDefinition {
    std::string sceneName;
    std::vector<SceneObjectDefinition> objectDefs;
    glm::vec3 initialCameraPosition{0.f, 0.f, 5.f};
    glm::vec3 initialCameraLookAt{0.f, 0.f, 0.f};
    polyscope::UpDir upDir;
    polyscope::FrontDir frontDir;
};

#endif // SCENE_DEFINITION_H