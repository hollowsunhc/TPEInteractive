#include "ExampleLoader.h"
#include "FCCLatticeSpheres.h"

#include <polyscope/polyscope.h>
#include <polyscope/view.h>

#include <stdexcept>

using EmbeddedData::two_spheres_vertex_count;
using EmbeddedData::two_spheres_simplex_count;
using EmbeddedData::two_spheres_vertex_coordinates;
using EmbeddedData::two_spheres_simplices;

using EmbeddedData::two_spheres_obstacle_vertex_count;
using EmbeddedData::two_spheres_obstacle_simplex_count;
using EmbeddedData::two_spheres_obstacle_vertex_coordinates;
using EmbeddedData::two_spheres_obstacle_simplices;

std::shared_ptr<MeshData> ExampleLoader::s_sphereTemplate = nullptr;
std::shared_ptr<MeshData> ExampleLoader::s_obstacleSphereTemplate = nullptr;


std::shared_ptr<MeshData> ExampleLoader::GetSphereTemplate() {
    if (!s_sphereTemplate) {
        polyscope::info("Loading sphere template data...");

        if (two_spheres_vertex_count == 0 || two_spheres_simplex_count == 0) {
            throw std::runtime_error("Global sphere template data not loaded/defined.");
        }

        s_sphereTemplate = std::make_shared<MeshData>();
        s_sphereTemplate->vertices.resize(two_spheres_vertex_count);
        for(size_t i = 0; i < two_spheres_vertex_count; ++i) {
            s_sphereTemplate->vertices[i] = { two_spheres_vertex_coordinates[i][0],
                                              two_spheres_vertex_coordinates[i][1],
                                              two_spheres_vertex_coordinates[i][2] };
        }
        s_sphereTemplate->simplices.resize(two_spheres_simplex_count);
        for(size_t i = 0; i < two_spheres_simplex_count; ++i) {
            s_sphereTemplate->simplices[i] = { two_spheres_simplices[i][0],
                                               two_spheres_simplices[i][1],
                                               two_spheres_simplices[i][2] };
        }
        polyscope::info("Sphere template loaded.");
    }
    return s_sphereTemplate;
}


std::shared_ptr<MeshData> ExampleLoader::GetObstacleSphereTemplate() {
    if (!s_obstacleSphereTemplate) {
        polyscope::info("Loading obstacle sphere template data from EmbeddedData...");

        if (two_spheres_obstacle_vertex_count == 0 || two_spheres_obstacle_simplex_count == 0) {
            throw std::runtime_error("Embedded obstacle sphere template data is empty or invalid.");
        }

        s_obstacleSphereTemplate = std::make_shared<MeshData>();
        s_obstacleSphereTemplate->vertices.resize(two_spheres_obstacle_vertex_count);
        for(size_t i = 0; i < two_spheres_obstacle_vertex_count; ++i) {
            s_obstacleSphereTemplate->vertices[i] = { two_spheres_obstacle_vertex_coordinates[i][0],
                                                      two_spheres_obstacle_vertex_coordinates[i][1],
                                                      two_spheres_obstacle_vertex_coordinates[i][2] };
        }
        s_obstacleSphereTemplate->simplices.resize(two_spheres_obstacle_simplex_count);
        for(size_t i = 0; i < two_spheres_obstacle_simplex_count; ++i) {
            s_obstacleSphereTemplate->simplices[i] = { two_spheres_obstacle_simplices[i][0],
                                                       two_spheres_obstacle_simplices[i][1],
                                                       two_spheres_obstacle_simplices[i][2] };
        }
        polyscope::info("Obstacle sphere template loaded.");
    }
    return s_obstacleSphereTemplate;
}


SceneDefinition ExampleLoader::LoadExample(ExampleId exampleId) {
    switch (exampleId) {
        case ExampleId::FCC_4:
            return CreateFCC4SphereScene();
        case ExampleId::TWO_SPHERES:
            return CreateTwoSphereScene();
        default:
            throw std::runtime_error("Unknown example ID requested.");
    }
}


SceneDefinition ExampleLoader::CreateFCC4SphereScene() {
    SceneDefinition scene;
    scene.sceneName = "FCC 4 Spheres";
    scene.upDir = polyscope::UpDir::YUp;
    scene.frontDir = polyscope::FrontDir::NegYFront;

    std::shared_ptr<MeshData> sphereMesh = GetSphereTemplate();

    // --- FCC Lattice Generation ---
    std::array<Real, 3> region_min = {-5., -5., 0.};
    std::array<Real, 3> region_max = {5.,  5.,  5.};
    double sphereRadius = 1.5;
    double centerSeparation = 2.0 * sphereRadius;
    double boundaryMargin = 0.12;
    auto raw_data_fcc = createFCCLatticeSpheres(
        region_min, region_max, sphereRadius, centerSeparation, boundaryMargin
    );
    // ---

    for (int i = 0; i < raw_data_fcc.size(); ++i) {
        auto uniqueMeshData = std::make_shared<MeshData>(*sphereMesh);
        uniqueMeshData->vertices = raw_data_fcc[i];

        scene.objectDefs.emplace_back(
            i,                      // id
            "sphere",               // baseName
            uniqueMeshData,         // meshData
            true,                   // isInteractive
            true,                   // isObstacleSource
            true,                   // isSimulated
            std::vector<int>{-1}    // obstacleDefinitionIds 
        );
    }

    // --- Set camera position and look at ---
    scene.initialCameraPosition = {0.f, 0.f, -7.f};
    scene.initialCameraLookAt = {0.f, 0.f, 2.5f};

    return scene;
}


SceneDefinition ExampleLoader::CreateTwoSphereScene() {
    SceneDefinition scene;
    scene.sceneName = "Two Spheres";
    scene.upDir = polyscope::UpDir::NegXUp;
    scene.frontDir = polyscope::FrontDir::NegYFront;

    std::shared_ptr<MeshData> mainSphereGeo = GetSphereTemplate();
    std::shared_ptr<MeshData> obstacleSphereGeo = GetObstacleSphereTemplate();

    // --- Object 0: Main Sphere ---
    scene.objectDefs.emplace_back(
        0,                      // id
        "sphere_main",          // baseName
        mainSphereGeo,          // meshData
        true,                   // isInteractive
        false,                  // isObstacleSource
        true,                   // isSimulated
        std::vector<int>{1}     // obstacleDefinitionIds (object 1)
    );

    // --- Object 1: Obstacle Sphere ---
    scene.objectDefs.emplace_back(
        1,                      // id
        "sphere_obstacle",      // baseName
        obstacleSphereGeo,      // meshData
        false,                  // isInteractive
        true,                   // isObstacleSource
        false,                  // isSimulated
        std::vector<int>{}      // obstacleDefinitionIds (none)
    );

    // --- Set camera position and look at ---
    scene.initialCameraPosition = {0.f, -3.15f, 0.f};
    scene.initialCameraLookAt = {0.f, 0.f, 0.f};

    return scene;
}