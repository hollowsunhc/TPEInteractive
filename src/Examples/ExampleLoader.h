#ifndef EXAMPLE_LOADER_H
#define EXAMPLE_LOADER_H

#include "../Data/SceneDefinition.h"

#include <memory>

class ExampleLoader {
public:
    static SceneDefinition LoadExample(ExampleId exampleId);

private:
    static SceneDefinition CreateFCC4SphereScene();
    static SceneDefinition CreateTwoSphereScene();

    // Helper to load/cache mesh templates
    static std::shared_ptr<MeshData> GetSphereTemplate();
    static std::shared_ptr<MeshData> GetObstacleSphereTemplate();
    static std::shared_ptr<MeshData> s_sphereTemplate;
    static std::shared_ptr<MeshData> s_obstacleSphereTemplate;
};

#endif // EXAMPLE_LOADER_H