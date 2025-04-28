#include "UIManager.h"

#include <imgui.h>
#include <polyscope/polyscope.h>  // For logging if needed

#include "../Application/Application.h"
#include "../Config/Config.h"
#include "../Scene/SceneManager.h"
#include "../Scene/SceneObject.h"
#include "../Utils/Helpers.h"

UIManager::UIManager(ConfigType& config, SceneManager& sceneManager, Application& application)
    : m_config(config), m_sceneManager(sceneManager), m_application(application) {
    m_exampleNamePtrs.reserve(m_exampleDisplayNames.size());
    for (const auto& pair : m_exampleDisplayNames) {
        m_exampleNamePtrs.push_back(pair.second.c_str());
    }
}

void UIManager::DrawUI() {
    ImGui::PushItemWidth(150);

    DrawExampleSelection();
    DrawInteractivityControls();
    DrawVectorVisualizationControls();
    DrawTPEControls();
    DrawActionControls();
    DrawDebugControls();

    ImGui::PopItemWidth();
}

void UIManager::DrawExampleSelection() {
    ImGui::Separator();
    ImGui::Text("Example Selection");

    int currentItem = static_cast<int>(m_currentSelectedExampleId);
    if (ImGui::Combo("Select Example", &currentItem, m_exampleNamePtrs.data(), m_exampleNamePtrs.size())) {
        ExampleId selectedId = static_cast<ExampleId>(currentItem);
        if (selectedId != m_currentSelectedExampleId) {
            m_currentSelectedExampleId = selectedId;
            m_application.RequestExampleLoad(m_currentSelectedExampleId);
            return;
        }
    }
    ImGui::SameLine();
    Utils::HelpMarker("Changing the example resets the simulation and visualization.");

    // Add Reset button
    if (ImGui::Button("Reset Current Example")) {
        m_application.RequestExampleLoad(m_currentSelectedExampleId);
    }
    ImGui::SameLine();
    Utils::HelpMarker("Reloads the currently selected example from scratch.");
}

void UIManager::DrawInteractivityControls() {
    ImGui::Separator();
    ImGui::Text("Interactive Controls");

    m_interactiveObjectNames.clear();
    m_interactiveObjectIds.clear();
    int activeComboIndex = -1;
    int currentActiveId = m_sceneManager.GetActiveObjectId();

    const auto& objects = m_sceneManager.GetObjects();
    for (int i = 0; i < objects.size(); ++i) {
        if (objects[i]->IsInteractive()) {
            const std::string& name = objects[i]->GetUniqueName();
            m_interactiveObjectNames.push_back(name.c_str());
            m_interactiveObjectIds.push_back(objects[i]->GetId());
            if (objects[i]->GetId() == currentActiveId) {
                activeComboIndex = static_cast<int>(m_interactiveObjectIds.size() - 1);
            }
        }
    }

    bool anyInteractive = !m_interactiveObjectNames.empty();
    ImGui::BeginDisabled(!anyInteractive);

    const char* previewLabel = (activeComboIndex != -1) ? m_interactiveObjectNames[activeComboIndex] : "None";
    if (ImGui::BeginCombo("Active Object", previewLabel)) {
        for (int i = 0; i < m_interactiveObjectNames.size(); ++i) {
            bool isSelected = (i == activeComboIndex);
            if (ImGui::Selectable(m_interactiveObjectNames[i], isSelected)) {
                if (i != activeComboIndex) {
                    m_sceneManager.SetActiveObjectId(m_interactiveObjectIds[i]);
                }
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
}

void UIManager::DrawVectorVisualizationControls() {
    ImGui::Separator();
    ImGui::Text("Vector Visualization");

    ImGui::Checkbox("Real-time Differentials", &m_config.Interactivity.realTimeDiff);
    ImGui::SameLine();
    Utils::HelpMarker("Updates differentials while dragging. Can be slow!");

    ImGui::BeginDisabled(!m_config.Interactivity.realTimeDiff);  // Grad depends on Diff
    if (ImGui::Checkbox("Real-time Gradients", &m_config.Interactivity.realTimeGrad)) {
        if (!m_config.Interactivity.realTimeDiff) {
            m_config.Interactivity.realTimeGrad = false;  // Auto-disable if diff is off
            polyscope::warning("Enable Real-time Differentials first.");
        }
    }
    ImGui::SameLine();
    Utils::HelpMarker("Updates gradients while dragging. Very slow! Requires Real-time Differentials.");
    ImGui::EndDisabled();

    // Scaling controls
    bool visualsNeedUpdate = false;
    visualsNeedUpdate |= ImGui::Checkbox("Logarithmic Vector Scale", &m_config.Display.useLogScale);
    ImGui::SameLine();
    Utils::HelpMarker("Scales vector lengths logarithmically. Ignores 'Linear Scale'.");

    ImGui::Indent();
    ImGui::BeginDisabled(!m_config.Display.useLogScale);  // Disable if log scale off
    visualsNeedUpdate |=
        ImGui::SliderFloat("Target Max Log Length", &m_config.Display.targetMaxLogScale, 0.1f, 5.0f, "%.1f");
    ImGui::EndDisabled();
    ImGui::Unindent();

    ImGui::BeginDisabled(m_config.Display.useLogScale);  // Disable if log scale on
    visualsNeedUpdate |=
        ImGui::SliderFloat("Linear Vector Scale", &m_config.Display.differentialScale, -1.0f, 10.0f, "%.2f");
    ImGui::EndDisabled();

    if (visualsNeedUpdate) {
        m_application.RequestVectorVisualsUpdate();
    }
}

void UIManager::DrawTPEControls() {
    ImGui::Separator();
    ImGui::Text("Repulsor TPE Settings");

    // TODO: To revise.

    // --- Basic Energy Params (Affect Energy/Metric Objects) ---
    bool pq_changed = false;
    pq_changed |= ImGui::InputDouble("q", &m_config.TPE.q, 0.1, 1.0, "%.1f");
    ImGui::SameLine();
    Utils::HelpMarker("Tangent point energy exponent q.");
    pq_changed |= ImGui::InputDouble("p", &m_config.TPE.p, 0.1, 1.0, "%.1f");
    ImGui::SameLine();
    Utils::HelpMarker("Tangent point energy exponent p.");

    if (pq_changed) {
        m_application.RequestRepulsorParamUpdate();
        // Note: RequestRepulsorParamUpdate might also trigger mesh param updates, which is fine.
    }

    // --- Mesh/Tree Params (Affect Mesh_T settings) ---
    bool mesh_params_changed = false;
    mesh_params_changed |= ImGui::InputDouble("Theta", &m_config.TPE.theta, 0.1, 0.0, "%.2f");  // Adapt step/format
    ImGui::SameLine();
    Utils::HelpMarker("Adaptivity parameter for far-field approximations.");
    mesh_params_changed |=
        ImGui::InputDouble("Intersection Theta", &m_config.TPE.intersection_theta, 100.0, 0.0, "%.1f");
    ImGui::SameLine();
    Utils::HelpMarker("Adaptivity parameter for intersection checks.");
    mesh_params_changed |= ImGui::InputDouble("Far Field Sep.", &m_config.TPE.farFieldSeparation, 0.01, 0.0, "%.3f");
    ImGui::SameLine();
    Utils::HelpMarker("Separation parameter for far-field blocks.");
    mesh_params_changed |= ImGui::InputDouble("Near Field Sep.", &m_config.TPE.nearFieldSeparation, 0.1, 0.0, "%.2f");
    ImGui::SameLine();
    Utils::HelpMarker("Separation parameter for near-field blocks.");
    mesh_params_changed |=
        ImGui::InputDouble("Near Field Inter.", &m_config.TPE.nearFieldIntersection, 100.0, 0.0, "%.1f");
    ImGui::SameLine();
    Utils::HelpMarker("Parameter for near-field intersection calculations.");
    mesh_params_changed |= ImGui::InputInt("Max Refinement", &m_config.TPE.maxRefinement);
    ImGui::SameLine();
    Utils::HelpMarker("Maximum depth of adaptive refinement.");
    mesh_params_changed |= ImGui::InputInt("Cluster Split Threshold", &m_config.TPE.clusterSplitThreshold);
    ImGui::SameLine();
    Utils::HelpMarker("Max cluster size before splitting.");
    mesh_params_changed |= ImGui::InputInt("Parallel Perc Depth", &m_config.TPE.parallelPercolationDepth);
    ImGui::SameLine();
    Utils::HelpMarker("Depth for parallel tree traversal.");
    mesh_params_changed |= ImGui::InputInt("Thread Count", &m_config.TPE.threadCount);
    ImGui::SameLine();
    Utils::HelpMarker("Number of threads for parallel processing.");

    if (mesh_params_changed) {
        m_application.RequestRepulsorParamUpdate();
    }
}

void UIManager::UpdateRepulsorParams() {
    m_application.RequestRepulsorParamUpdate();
}

void UIManager::DrawActionControls() {
    ImGui::Separator();
    ImGui::Text("Actions");
    ImGui::InputInt("Loop iterations", &m_config.Opt.nLoopIterations);
    ImGui::SameLine();
    Utils::HelpMarker("Number of physics steps per button click.");

    if (ImGui::Button("Update Mesh")) {
        m_application.RequestPhysicsStep(m_config.Opt.nLoopIterations);
    }
    ImGui::SameLine();
    Utils::HelpMarker("Displaces mesh coordinates in the direction minimizing the tangent point energy.");

    if (ImGui::Button("Print Energy")) {
        m_application.RequestPrintEnergy();
    }
    ImGui::SameLine();
    Utils::HelpMarker("Prints energies to console.");

    if (ImGui::Button("Calculate Differential")) {
        m_application.RequestCalculateAndShowDifferential();
    }
    ImGui::SameLine();
    Utils::HelpMarker("Calculates the energy differential vectors. Creates and/or updates visuals.");

    if (ImGui::Button("Calculate Gradient")) {
        m_application.RequestCalculateAndShowGradient();
    }
    ImGui::SameLine();
    Utils::HelpMarker("Calculates the energy gradient vectors. Creates and/or updates visuals. If "
                      "differentials are invalid, calculates them beforehand.");
}

void UIManager::DrawDebugControls() {
    ImGui::Separator();
    ImGui::Text("Debugging");

    if (ImGui::Checkbox("Show Obstacle Meshes", &m_config.Display.showObstacles)) {
        m_application.RequestObstacleVisualToggle(m_config.Display.showObstacles);
    }
    ImGui::SameLine();
    Utils::HelpMarker("Toggles the visualization of the combined obstacle meshes used by Repulsor.");

    ImGui::PushItemWidth(110);
    const char* verbosityLevels[] = {"Disabled (0)", "Info (1)", "Debug (2)"};
    int currentVerbosity = m_config.Debug.verbosity;
    if (currentVerbosity < 0) {
        currentVerbosity = 0;
    }
    if (currentVerbosity > 2) {
        currentVerbosity = 2;
    }

    if (ImGui::Combo("Log Verbosity", &currentVerbosity, verbosityLevels, IM_ARRAYSIZE(verbosityLevels))) {
        if (currentVerbosity != m_config.Debug.verbosity) {
            m_config.Debug.verbosity = currentVerbosity;
            m_application.RequestVerbosityUpdate(m_config.Debug.verbosity);
        }
    }
    ImGui::SameLine();
    Utils::HelpMarker("Controls the amount of log output from Polyscope and the application.");
    ImGui::PopItemWidth();

    SceneObject* activeObj = m_sceneManager.GetActiveObject();
    ImGui::BeginDisabled(!activeObj);  // Disable if no active object
    if (ImGui::Button("Recreate Mesh from TPEMeshPtr")) {
        if (activeObj) {
            m_application.RequestDebugMeshCreation(activeObj->GetId());
        }
    }
    ImGui::SameLine();
    Utils::HelpMarker("Creates a new mesh using the current coordinates stored inside the selected "
                      "Repulsor mesh object.");
    ImGui::EndDisabled();

    if (ImGui::Button("Print Camera Position")) {
        glm::vec3 pos = polyscope::view::getCameraWorldPosition();
        polyscope::info("Camera Position: " + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " +
                        std::to_string(pos.z));
    }
    ImGui::SameLine();
    Utils::HelpMarker("Prints the current camera position to the console.");
}
