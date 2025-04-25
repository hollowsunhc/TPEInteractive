#ifndef APPLICATION_H
#define APPLICATION_H

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <vector>

#include "../Config/Config.h"
#include "../Data/SceneDefinition.h"
#include "../Engine/RepulsorEngine.h"
#include "../Engine/VisualizationEngine.h"
#include "../Scene/SceneManager.h"
#include "../UI/UIManager.h"

struct VizCalculationCache {
    // Store data needed for visualization
    std::vector<glm::vec3> diff_glm;
    std::vector<glm::vec3> grad_glm;
    bool diff_valid = false;
    bool grad_valid = false;
    // TODO: (maybe?) Optionally store raw tensors if needed elsewhere
    // Tensors::Tensor2<Real, Int> differential;
    // Tensors::Tensor2<Real, Int> gradient;
};

class Application {
  public:
    Application();
    ~Application();

    void Initialize(int argc, char** argv);
    void Run();

    // --- Callbacks / Event Handlers ---
    void MainLoopIteration();  // Called by Polyscope each frame
    void CheckGizmoInteraction();

    // --- Actions Triggered by UI ---
    void RequestExampleLoad(ExampleId exampleId);
    void RequestPhysicsStep(int iterations);
    void RequestRepulsorParamUpdate();
    void RequestPrintEnergy();
    void RequestDebugMeshCreation(int objectId);
    void RequestCalculateAndShowDifferential();
    void RequestCalculateAndShowGradient();
    void RequestVectorVisualsUpdate();
    void RequestObstacleVisualToggle(bool show);
    void RequestVerbosityUpdate(int newLevel);

  private:
    void SetupPolyscope();
    void LoadInitialScene();
    void CalculateAllDifferentialsInternal();
    void CalculateAllGradientsInternal();
    void UpdateDifferentialVisualsInternal();
    void UpdateGradientVisualsInternal();
    void InvalidateCalculationCache();

    ConfigType m_config;

    std::unique_ptr<RepulsorEngine> m_repulsorEngine;
    std::unique_ptr<VisualizationEngine> m_vizEngine;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<UIManager> m_uiManager;

    ExampleId m_currentExample = ExampleId::FCC_4;

    std::map<int, VizCalculationCache> m_vizCache;
    bool m_globalDiffValid = false;
    bool m_globalGradValid = false;
};

#endif  // APPLICATION_H