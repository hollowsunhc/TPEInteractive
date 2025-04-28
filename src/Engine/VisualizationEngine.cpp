#include "VisualizationEngine.h"

#include <polyscope/polyscope.h>
#include <polyscope/surface_mesh.h>
#include <polyscope/surface_vector_quantity.h>
#include <polyscope/vector_quantity.h>
#include <polyscope/view.h>

#include "../Config/Config.h"
#include "../Scene/SceneManager.h"
#include "../Scene/SceneObject.h"  // Full definition
#include "../Utils/Helpers.h"      // For scaling etc.

VisualizationEngine::VisualizationEngine(const ConfigType& config) : m_config(config) {
    polyscope::info("Initializing Visualization Engine (Polyscope)...");
}

bool VisualizationEngine::RegisterObject(SceneObject& object) {
    const auto& vertices = object.GetInitialVertices();
    const auto& simplices = object.GetSimplices();
    const std::string& name = object.GetUniqueName();

    if (vertices.empty() || simplices.empty()) {
        polyscope::warning("VizEngine: Skipping registration for " + name + " (empty geometry).");
        return false;
    }

    try {
        polyscope::removeStructure(name, false);

        auto* psMesh = polyscope::registerSurfaceMesh(name, vertices, simplices);
        if (psMesh) {
            polyscope::info("Successfully registered Polyscope mesh: '" + name + "'");
        } else {
            polyscope::error("Polyscope registration returned null for: '" + name + "'");
        }

        psMesh->setTransform(object.GetCurrentTransform());
        psMesh->setTransparency(0.8f);
        psMesh->setEnabled(true);
        psMesh->setSmoothShade(true);
        psMesh->setTransformGizmoEnabled(false);

        polyscope::info("VizEngine: Registered object " + name);
        return true;

    } catch (const std::exception& e) {
        polyscope::error("VizEngine: Failed to register object " + name + ": " + std::string(e.what()));
        return false;
    }
}

bool VisualizationEngine::RemoveObjectByName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    polyscope::info("VizEngine: Requesting removal of structure '" + name + "'");

    std::string obsName = name + "_Obstacle";
    polyscope::removeStructure(obsName);
    polyscope::removeStructure(name);

    return true;
}

void VisualizationEngine::RemoveAllObjects() {
    polyscope::info("VizEngine: Removing all structures.");
    polyscope::removeAllStructures();
}

void VisualizationEngine::UpdateObjectTransform(SceneObject& object) {
    const std::string& name = object.GetUniqueName();
    auto* psMesh = polyscope::getSurfaceMesh(name);
    if (psMesh) {
        psMesh->setTransform(object.GetCurrentTransform());
    } else {
        polyscope::warning("VizEngine: Could not find structure " + name + " to update transform.");
    }
}

void VisualizationEngine::UpdateObjectVertices(SceneObject& object) {
    const std::string& name = object.GetUniqueName();
    auto* psMesh = polyscope::getSurfaceMesh(name);
    if (psMesh) {
        psMesh->updateVertexPositions(object.GetInitialVertices());  // Update with local coords
    } else {
        polyscope::warning("VizEngine: Could not find structure " + name + " to update vertices.");
    }
}

void VisualizationEngine::UpdateActiveGizmo(const std::string& oldActiveName, const std::string& newActiveName) {
    // Disable old
    if (!oldActiveName.empty() && oldActiveName != newActiveName) {
        auto* oldStruct = polyscope::getSurfaceMesh(oldActiveName);
        if (oldStruct) {
            oldStruct->setTransformGizmoEnabled(false);
        } else {
            polyscope::warning("VizEngine: Could not find old structure '" + oldActiveName + "' to disable gizmo.");
        }
    }

    // Enable new
    if (!newActiveName.empty()) {
        auto* newStruct = polyscope::getSurfaceMesh(newActiveName);
        if (newStruct) {
            newStruct->setTransformGizmoEnabled(true);
            polyscope::info("VizEngine: Enabled gizmo for " + newActiveName);
        } else {
            polyscope::warning("VizEngine: Could not find new structure '" + newActiveName + "' to enable gizmo.");
        }
    } else {
        polyscope::info("VizEngine: All gizmos disabled.");
    }
    RequestRedraw();
}

void VisualizationEngine::UpdateVectorQuantity(SceneObject& object, const std::string& quantityName,
                                               const std::vector<glm::vec3>& vectors) {
    const std::string& meshName = object.GetUniqueName();
    polyscope::info("VizEngine: UpdateVectorQuantity START - Name: " + meshName + ", QName: " + quantityName +
                    ", VecCount: " + std::to_string(vectors.size()));

    auto* psMesh = polyscope::getSurfaceMesh(meshName);
    if (!psMesh) {
        polyscope::warning("  UpdateVectorQuantity: Polyscope mesh '" + meshName + "' not found.");
        return;
    }

    std::vector<glm::vec3> scaledVectors = Utils::scaleVectorsForVisualization(
        vectors, m_config.Display.useLogScale, m_config.Display.differentialScale, m_config.Display.targetMaxLogScale);

    // We're only dealing with surface mesh vector quantities for now.
    polyscope::SurfaceMeshQuantity* diffQuantitySV = psMesh->getQuantity(quantityName);

    if (diffQuantitySV) {
        auto diffQuantityV =
            dynamic_cast<polyscope::VectorQuantity<polyscope::SurfaceVertexVectorQuantity>*>(diffQuantitySV);
        diffQuantityV->updateData(scaledVectors);
    } else {
        psMesh->addVertexVectorQuantity(quantityName, scaledVectors, polyscope::VectorType::AMBIENT)
            ->setVectorRadius(0.01)
            ->setEnabled(true);
    }
}

void VisualizationEngine::RemoveVectorQuantity(SceneObject& object, const std::string& quantityName) {
    const std::string& meshName = object.GetUniqueName();
    auto* psMesh = polyscope::getSurfaceMesh(meshName);
    if (psMesh) {
        psMesh->removeQuantity(quantityName, false);
    }
}

void VisualizationEngine::RemoveAllVectorQuantities(SceneObject& object) {
    const std::string& meshName = object.GetUniqueName();
    auto* psMesh = polyscope::getSurfaceMesh(meshName);
    if (psMesh) {
        psMesh->removeQuantity("Differential", false);
        psMesh->removeQuantity("Gradient", false);
    }
}

void VisualizationEngine::RequestRedraw() {
    polyscope::requestRedraw();
}

void VisualizationEngine::SetCameraView(const glm::vec3& position, const glm::vec3& lookAt, polyscope::UpDir upDir,
                                        polyscope::FrontDir frontDir) {
    polyscope::view::upDir = upDir;
    polyscope::view::frontDir = frontDir;
    polyscope::view::lookAt(position, lookAt);
}

void VisualizationEngine::ResetCamera() {
    polyscope::view::resetCameraToHomeView();
}

void VisualizationEngine::UpdateSingleObstacleVisual(SceneObject& targetObject) {
    if (!targetObject.IsSimulated() || !targetObject.GetRepulsorMesh()) {
        return;
    }

    std::string obsName = targetObject.GetUniqueName() + "_Obstacle";

    const Mesh_T* obsMeshPtr = nullptr;
    try {
        Mesh_T* mainMesh = targetObject.GetRepulsorMesh();
        if (mainMesh) {
            const Mesh_T& restrict obsMeshRef = mainMesh->GetObstacle();
            obsMeshPtr = &obsMeshRef;
        }
    } catch (const std::exception& e) {
        // Handle cases where GetObstacle() might throw if none is loaded
        polyscope::info("UpdateSingleObstacleVisual: No obstacle found for " + targetObject.GetUniqueName() +
                        " (exception: " + e.what() + ")");
        obsMeshPtr = nullptr;
    } catch (...) {  // Catch potential non-std exceptions from GetObstacle
        polyscope::info("UpdateSingleObstacleVisual: No obstacle found (non-std exception) for " +
                        targetObject.GetUniqueName());
        obsMeshPtr = nullptr;
    }

    bool obstacleDataIsValid = obsMeshPtr && obsMeshPtr->VertexCount() > 0 && obsMeshPtr->SimplexCount() > 0;
    bool hasPsObsMesh = polyscope::hasSurfaceMesh(obsName);

    if (obstacleDataIsValid) {
        std::vector<std::array<Real, 3>> verts;
        std::vector<std::vector<Int>> faces;

        try {
            verts = Utils::tensorToVecArray(obsMeshPtr->VertexCoordinates());

            const auto& simplexTensor = obsMeshPtr->Simplices();
            Int nSimplex = simplexTensor.Dimension(0);
            Int vertsPerSimplex = simplexTensor.Dimension(1);

            if (vertsPerSimplex == (dom_dim + 1)) {
                faces.resize(nSimplex);
                for (Int i = 0; i < nSimplex; ++i) {
                    faces[i].resize(vertsPerSimplex);
                    for (Int j = 0; j < vertsPerSimplex; ++j) {
                        faces[i][j] = simplexTensor(i, j);
                    }
                }
            } else {
                polyscope::error("Obstacle simplex data has unexpected dimension: " + std::to_string(vertsPerSimplex));
            }

            if (verts.empty() || faces.empty()) {
                throw std::runtime_error("Empty geometry after extraction.");
            }

            if (hasPsObsMesh) {
                auto* psObsMesh = polyscope::getSurfaceMesh(obsName);
                psObsMesh->updateVertexPositions(verts);
            } else {
                auto* psObsMesh = polyscope::registerSurfaceMesh(obsName, verts, faces);
                if (!psObsMesh) {
                    throw std::runtime_error("registerSurfaceMesh failed");
                }
                psObsMesh->setEnabled(m_config.Display.showObstacles);
                psObsMesh->setTransparency(0.6f);
                psObsMesh->setShadeStyle(polyscope::MeshShadeStyle::Smooth);
                psObsMesh->setTransformGizmoEnabled(false);
                psObsMesh->setSurfaceColor({0.8f, 0.5f, 0.5f});  // gray
                polyscope::info("Registered obstacle visual: " + obsName);
            }

        } catch (const std::exception& e) {
            polyscope::error("VizEngine: Failed to register/update obstacle visual " + obsName + ": " +
                             std::string(e.what()));
            polyscope::removeStructure(obsName, false);
        }
    } else {
        if (hasPsObsMesh) {
            polyscope::removeStructure(obsName);
            polyscope::info("Removed obstacle visual: " + obsName);
        }
    }
}

void VisualizationEngine::ShowObstaclesForAll(const std::vector<std::unique_ptr<SceneObject>>& objects) {
    polyscope::info("VizEngine: Enabling obstacle visuals...");
    for (const auto& objPtr : objects) {
        if (!objPtr->IsSimulated()) {
            continue;
        }

        // Call UpdateSingleObstacleVisual first to ensure structure exists if data is valid
        UpdateSingleObstacleVisual(*objPtr);  // This might register it if needed

        std::string obsName = objPtr->GetUniqueName() + "_Obstacle";
        auto* psObsMesh = polyscope::getSurfaceMesh(obsName);
        if (psObsMesh) {
            psObsMesh->setEnabled(true);
        }
    }
    RequestRedraw();
}

void VisualizationEngine::HideObstaclesForAll(const std::vector<std::unique_ptr<SceneObject>>& objects) {
    polyscope::info("VizEngine: Disabling obstacle visuals...");
    for (const auto& objPtr : objects) {
        if (!objPtr->IsSimulated()) {
            continue;
        }

        std::string obsName = objPtr->GetUniqueName() + "_Obstacle";
        auto* psObsMesh = polyscope::getSurfaceMesh(obsName);
        if (psObsMesh) {
            psObsMesh->setEnabled(false);
        }
    }
    RequestRedraw();
}
