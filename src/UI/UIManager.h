#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "../Data/SceneDefinition.h"

#include <string>
#include <vector>
#include <map>

struct ConfigType;
class SceneManager;
class Application;


class UIManager {
public:
    UIManager(ConfigType& config, SceneManager& sceneManager, Application& application);
    ~UIManager() = default;

    void DrawUI(); // Main drawing function called by Application

private:
    void DrawExampleSelection();
    void DrawInteractivityControls();
    void DrawVectorVisualizationControls();
    void DrawTPEControls();
    void DrawActionControls();
    void DrawDebugControls();

    void UpdateRepulsorParams();

    ConfigType& m_config;
    SceneManager& m_sceneManager;
    Application& m_application;

    // Example management data
    const std::map<ExampleId, std::string> m_exampleDisplayNames = {
        { ExampleId::FCC_4, "FCC 4 Spheres" },
        { ExampleId::TWO_SPHERES,   "Two Spheres"   }
    };
    std::vector<const char*> m_exampleNamePtrs;
    ExampleId m_currentSelectedExampleId = ExampleId::FCC_4;

    // Cache for combo boxes etc.
    std::vector<const char*> m_interactiveObjectNames;
    std::vector<int> m_interactiveObjectIds;

};

#endif // UI_MANAGER_H