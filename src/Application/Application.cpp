#include "Application.h"

#include <polyscope/options.h>
#include <polyscope/polyscope.h>
#include <polyscope/surface_mesh.h>

#include <iostream>

#include "../Examples/ExampleLoader.h"
#include "../Scene/SceneObject.h"
#include "../Utils/Helpers.h"

namespace {  // Anonymous namespace for file-local scope
Application* g_appInstance = nullptr;

void PolyscopeCallback() {
    if (g_appInstance) {
        g_appInstance->MainLoopIteration();
    }
}
}  // namespace

Application::Application() {
    if (g_appInstance) {
        throw std::runtime_error("Application instance already exists.");
    }
    g_appInstance = this;
}

Application::~Application() {
    g_appInstance = nullptr;
    polyscope::shutdown();
}

void Application::Initialize(int argc, char** argv) {
    polyscope::options::programName = "TPE Interactive";
    polyscope::options::verbosity = m_config.Debug.verbosity;
    polyscope::init();

    m_repulsorEngine = std::make_unique<RepulsorEngine>(m_config);
    m_vizEngine = std::make_unique<VisualizationEngine>(m_config);
    m_sceneManager = std::make_unique<SceneManager>(*m_repulsorEngine, *m_vizEngine, m_config);
    m_uiManager = std::make_unique<UIManager>(m_config, *m_sceneManager, *this);

    SetupPolyscope();
    LoadInitialScene();
}

void Application::SetupPolyscope() {
    polyscope::state::userCallback = PolyscopeCallback;
}

void Application::LoadInitialScene() {
    RequestExampleLoad(m_currentExample);
}

void Application::Run() {
    polyscope::show();
}

void Application::MainLoopIteration() {
    m_uiManager->DrawUI();
    CheckGizmoInteraction();
}

void Application::CheckGizmoInteraction() {
    int activeObjId = m_sceneManager->GetActiveObjectId();
    if (activeObjId == -1) {
        return;  // No active object
    }

    SceneObject* activeObj = m_sceneManager->GetObjectById(activeObjId);
    if (!activeObj || !activeObj->IsInteractive()) {
        return;
    }

    auto* psMesh = polyscope::getSurfaceMesh(activeObj->GetUniqueName());

    if (psMesh) {
        glm::mat4 currentGizmoTransform = psMesh->getTransform();

        if (!Utils::matricesAreClose(currentGizmoTransform, activeObj->GetCurrentTransform())) {

            m_sceneManager->UpdateObjectTransform(activeObjId, currentGizmoTransform);
            InvalidateCalculationCache();

            if (m_config.Interactivity.realTimeDiff) {
                CalculateAllDifferentialsInternal();
                UpdateDifferentialVisualsInternal();
                if (m_config.Interactivity.realTimeGrad) {
                    CalculateAllGradientsInternal();
                    UpdateGradientVisualsInternal();
                }
            }
            m_vizEngine->RequestRedraw();
        }
    }
}

void Application::RequestExampleLoad(ExampleId exampleId) {
    polyscope::info("Application: Requesting load for example ID: " + std::to_string(static_cast<int>(exampleId)));
    m_currentExample = exampleId;
    try {
        m_vizEngine->RemoveAllObjects();
        InvalidateCalculationCache();
        SceneDefinition sceneDef = ExampleLoader::LoadExample(exampleId);
        m_vizEngine->SetCameraView(sceneDef.initialCameraPosition, sceneDef.initialCameraLookAt, sceneDef.upDir,
                                   sceneDef.frontDir);
        bool loaded = m_sceneManager->LoadScene(sceneDef);
        if (!loaded) {
            polyscope::error("Application: Scene loading failed.");
        } else {
            polyscope::info("Application: Scene loaded successfully.");
        }
    } catch (const std::exception& e) {
        polyscope::error("Application: Exception during example load: " + std::string(e.what()));
        m_sceneManager->UnloadScene();
    }
    m_vizEngine->RequestRedraw();
}

void Application::RequestPhysicsStep(int iterations) {
    if (iterations <= 0) {
        return;
    }
    InvalidateCalculationCache();
    m_sceneManager->ApplyPhysicsStep(iterations);

    bool visuals_updated = false;
    if (m_config.Interactivity.realTimeDiff) {
        polyscope::info("Application: Recalculating Differential after physics step (real-time enabled)...");
        CalculateAllDifferentialsInternal();
        UpdateDifferentialVisualsInternal();
        visuals_updated = true;

        if (m_config.Interactivity.realTimeGrad) {
            polyscope::info("Application: Recalculating Gradient after physics step (real-time enabled)...");
            if (m_globalDiffValid) {
                CalculateAllGradientsInternal();
                UpdateGradientVisualsInternal();
            } else {
                polyscope::warning("Application: Skipping post-step gradient calculation as differential failed.");
                UpdateGradientVisualsInternal();  // This will remove visuals if m_globalGradValid
                                                  // is false
            }
            visuals_updated = true;
        }
    }

    if (!visuals_updated) {
        m_vizEngine->RequestRedraw();
    }
}

void Application::RequestRepulsorParamUpdate() {
    polyscope::info("Application: Repulsor parameter update requested.");
    InvalidateCalculationCache();
    m_sceneManager->UpdateEngineParametersForAllObjects();
}

void Application::RequestPrintEnergy() {
    polyscope::info("--- Energy Report ---");
    for (const auto& objPtr : m_sceneManager->GetObjects()) {
        if (objPtr->IsSimulated()) {
            try {
                Real energy = m_repulsorEngine->GetEnergy(*objPtr);
                polyscope::info(objPtr->GetUniqueName() + ": " + std::to_string(energy));
            } catch (...) {
                polyscope::info(objPtr->GetUniqueName() + ": Error calculating energy.");
            }
        }
    }
    polyscope::info("---------------------");
}

void Application::RequestDebugMeshCreation(int objectId) {
    SceneObject* obj = m_sceneManager->GetObjectById(objectId);
    if (!obj || !obj->IsSimulated() || !obj->GetRepulsorMesh()) {
        polyscope::warning("Debug Mesh Request: Invalid object or no Repulsor mesh for ID " + std::to_string(objectId));
        return;
    }

    std::string debugName = obj->GetUniqueName() + "_DebugState";
    polyscope::info("Requesting debug mesh creation: " + debugName);

    try {
        Tensors::Tensor2<Real, Int> worldCoords = obj->GetRepulsorMesh()->VertexCoordinates();
        if (worldCoords.Dimension(0) == 0) {
            polyscope::warning("Debug Mesh Request: Repulsor mesh has no vertices.");
            return;
        }

        std::vector<std::array<Real, 3>> verts = Utils::tensorToVecArray(worldCoords);
        const auto& faces = obj->GetSimplices();

        if (verts.empty() || faces.empty()) {
            polyscope::warning("Debug Mesh Request: Cannot create debug mesh with empty geometry.");
            return;
        }

        polyscope::removeStructure(debugName, false);

        auto* psDebugMesh = polyscope::registerSurfaceMesh(debugName, verts, faces);
        psDebugMesh->setEnabled(true);
        psDebugMesh->setSurfaceColor({0.2f, 0.8f, 0.2f});
        psDebugMesh->setTransparency(0.7f);
        psDebugMesh->setSmoothShade(true);
        psDebugMesh->setTransform(glm::mat4(1.0f));
        psDebugMesh->setTransformGizmoEnabled(false);

    } catch (const std::exception& e) {
        polyscope::error("Failed to create debug mesh " + debugName + ": " + std::string(e.what()));
        polyscope::removeStructure(debugName, false);
    }
    m_vizEngine->RequestRedraw();
}

void Application::InvalidateCalculationCache() {
    polyscope::info("Application: Invalidating calculation cache...");
    m_vizCache.clear();
    m_globalDiffValid = false;
    m_globalGradValid = false;
}

void Application::CalculateAllDifferentialsInternal() {
    polyscope::info("Application: Calculating all differentials...");
    if (!m_repulsorEngine || !m_sceneManager) {
        return;
    }

    InvalidateCalculationCache();
    bool all_ok = true;

    for (const auto& objPtr : m_sceneManager->GetObjects()) {
        if (!objPtr->IsSimulated() || !objPtr->GetRepulsorMesh()) {
            continue;
        }

        int id = objPtr->GetId();
        m_vizCache[id] = VizCalculationCache();

        try {
            Tensors::Tensor2<Real, Int> diff_tensor =
                m_repulsorEngine->GetDifferential(*objPtr);
            m_vizCache[id].diff_glm = Utils::tensorToGlmVec3(diff_tensor);
            m_vizCache[id].diff_valid = true;

        } catch (const std::exception& e) {
            polyscope::error("Application: Failed differential calc for " + objPtr->GetUniqueName() + ": " + e.what());
            m_vizCache[id].diff_valid = false;
            all_ok = false;
        }
    }
    m_globalDiffValid = all_ok;
    polyscope::info("Differential calculation complete. Overall validity: " +
                    std::string(m_globalDiffValid ? "OK" : "FAILED"));
}

void Application::CalculateAllGradientsInternal() {
    polyscope::info("Application: Calculating all gradients...");
    if (!m_repulsorEngine || !m_sceneManager) {
        return;
    }

    if (!m_globalDiffValid) {
        polyscope::warning("Application: Cannot calculate gradients, differentials invalid.");
        m_globalGradValid = false;  // Ensure grad is marked invalid
        for (auto& pair : m_vizCache) {
            pair.second.grad_valid = false;
        }  // Mark all cache entries
        return;
    }

    bool all_ok = true;
    for (const auto& objPtr : m_sceneManager->GetObjects()) {
        if (!objPtr->IsSimulated() || !objPtr->GetRepulsorMesh()) {
            continue;
        }

        int id = objPtr->GetId();

        if (!m_vizCache.count(id) || !m_vizCache[id].diff_valid) {
            polyscope::warning("Application: Skipping gradient for " + objPtr->GetUniqueName() +
                               ", differential invalid/missing.");
            m_vizCache[id].grad_valid = false;
            all_ok = false;
            continue;
        }

        try {
            Tensors::Tensor2<Real, Int> grad_tensor = m_repulsorEngine->GetGradient(*objPtr);
            m_vizCache[id].grad_glm = Utils::tensorToGlmVec3(grad_tensor);
            m_vizCache[id].grad_valid = true;

        } catch (const std::exception& e) {
            polyscope::error("Application: Failed gradient calc for " + objPtr->GetUniqueName() + ": " + e.what());
            m_vizCache[id].grad_valid = false;
            all_ok = false;
        }
    }
    m_globalGradValid = all_ok;
    polyscope::info("Gradient calculation complete. Overall validity: " +
                    std::string(m_globalGradValid ? "OK" : "FAILED"));
}

void Application::UpdateDifferentialVisualsInternal() {
    polyscope::info("Application: Updating differential visuals...");
    if (!m_vizEngine || !m_sceneManager) {
        return;
    }

    for (const auto& objPtr : m_sceneManager->GetObjects()) {
        int id = objPtr->GetId();
        if (m_vizCache.count(id) && m_vizCache[id].diff_valid) {
            m_vizEngine->UpdateVectorQuantity(*objPtr, "Differential", m_vizCache[id].diff_glm);
        } else {
            m_vizEngine->RemoveVectorQuantity(*objPtr, "Differential");
        }
    }
    m_vizEngine->RequestRedraw();
}

void Application::UpdateGradientVisualsInternal() {
    polyscope::info("Application: Updating gradient visuals...");
    if (!m_vizEngine || !m_sceneManager) {
        return;
    }

    for (const auto& objPtr : m_sceneManager->GetObjects()) {
        int id = objPtr->GetId();
        if (m_vizCache.count(id) && m_vizCache[id].grad_valid) {
            m_vizEngine->UpdateVectorQuantity(*objPtr, "Gradient", m_vizCache[id].grad_glm);
        } else {
            m_vizEngine->RemoveVectorQuantity(*objPtr, "Gradient");
        }
    }
    m_vizEngine->RequestRedraw();
}

void Application::RequestCalculateAndShowDifferential() {
    CalculateAllDifferentialsInternal();
    UpdateDifferentialVisualsInternal();
}

void Application::RequestCalculateAndShowGradient() {
    if (!m_globalDiffValid) {
        CalculateAllDifferentialsInternal();
        UpdateDifferentialVisualsInternal();
    }

    if (m_globalDiffValid) {
        if (!m_globalGradValid) {
            CalculateAllGradientsInternal();
            UpdateGradientVisualsInternal();
        }
    } else {
        polyscope::warning("Application: Cannot show gradient, differential calculation failed.");
        // Ensure gradient visuals are removed if diff failed
        UpdateGradientVisualsInternal();
    }
}

void Application::RequestVectorVisualsUpdate() {
    // Called when display config (log scale, linear scale) changes
    polyscope::info("Application: Updating vector visuals based on display settings...");
    UpdateDifferentialVisualsInternal();
    UpdateGradientVisualsInternal();
}

void Application::RequestObstacleVisualToggle(bool show) {
    m_config.Display.showObstacles = show;
    if (show) {
        m_vizEngine->ShowObstaclesForAll(m_sceneManager->GetObjects());
    } else {
        m_vizEngine->HideObstaclesForAll(m_sceneManager->GetObjects());
    }
}

void Application::RequestVerbosityUpdate(int newLevel) {
    // Clamp level just in case
    if (newLevel < 0) {
        newLevel = 0;
    }
    if (newLevel > 2) {
        newLevel = 2;  // Polyscope typically uses 0, 1, 2
    }

    polyscope::options::verbosity = newLevel;
    m_config.Debug.verbosity = newLevel;  // Ensure config stays in sync
    polyscope::info("Application: Verbosity set to " + std::to_string(newLevel));
}
