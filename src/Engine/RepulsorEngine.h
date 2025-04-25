#ifndef REPULSOR_ENGINE_H
#define REPULSOR_ENGINE_H

#include <array>
#include <memory>
#include <mutex>
#include <vector>

#include "../Config/Config.h"
#include "../Utils/GlobalTypes.h"

class SceneObject;
namespace Utils {
struct CombinedObstacleGeometry;
}

class RepulsorEngine {
  public:
    explicit RepulsorEngine(const ConfigType& config);
    ~RepulsorEngine();

    // --- Mesh Management ---
    bool InitializeRepulsorMesh(SceneObject& object);
    bool UpdateRepulsorMeshState(SceneObject& object);
    void ApplyCurrentConfigToMesh(SceneObject& object);
    std::unique_ptr<Mesh_T> CreateObstacleMesh(const std::vector<std::array<Real, 3>>& vertices,
                                               const std::vector<std::array<Int, 3>>& simplices);

    // --- Physics Calculations ---
    Tensors::Tensor2<Real, Int> CalculateWorldDisplacement(SceneObject& object);
    Tensors::Tensor2<Real, Int> GetDifferential(SceneObject& object);
    Tensors::Tensor2<Real, Int> GetGradient(SceneObject& object);
    Real GetEnergy(SceneObject& object);

    // --- Parameter Updates ---
    void UpdateEngineParameters();  // Called when config changes

  private:
    void UpdateMeshParametersInternal(Mesh_T* meshPtr);
    void CreateOrUpdateEnergyMetricObjects();

    const ConfigType& m_config;

    // Factories owned by the engine
    std::unique_ptr<TPE_Factory_T> m_tpeFactory;
    std::unique_ptr<TPM_Factory_T> m_tpmFactory;

    // Energy/Metric objects
    std::unique_ptr<Energy_T> m_energyObj;
    std::unique_ptr<Metric_T> m_metricObj;
    double m_current_p = -1.0;
    double m_current_q = -1.0;
    std::mutex m_energyMetricMutex;
};

#endif  // REPULSOR_ENGINE_H