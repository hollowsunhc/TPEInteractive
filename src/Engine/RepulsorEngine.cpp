#include "RepulsorEngine.h"

#include <polyscope/polyscope.h>

#include <stdexcept>

#include "../Scene/SceneObject.h"
#include "../Utils/Helpers.h"

RepulsorEngine::RepulsorEngine(const ConfigType& config) : m_config(config) {
    polyscope::info("Initializing Repulsor Engine...");
    try {
        m_tpeFactory = std::make_unique<TPE_Factory_T>();
        m_tpmFactory = std::make_unique<TPM_Factory_T>();
        CreateOrUpdateEnergyMetricObjects();
    } catch (const std::exception& e) {
        polyscope::error("Failed to create Repulsor factories: " + std::string(e.what()));
        throw;
    }
    polyscope::info("Repulsor Engine Initialized.");
}

RepulsorEngine::~RepulsorEngine() {
    polyscope::info("Shutting down Repulsor Engine.");
}

void RepulsorEngine::CreateOrUpdateEnergyMetricObjects() {
    std::lock_guard<std::mutex> lock(m_energyMetricMutex);

    if (!m_energyObj || !m_metricObj || m_current_p != m_config.TPE.p || m_current_q != m_config.TPE.q) {
        polyscope::info("Updating Repulsor energy/metric objects (p=" + std::to_string(m_config.TPE.p) +
                        ", q=" + std::to_string(m_config.TPE.q) + ")");
        try {
            m_energyObj = m_tpeFactory->Make(dom_dim, dom_dim, amb_dim, m_config.TPE.q, m_config.TPE.p);
            m_metricObj = m_tpmFactory->Make(dom_dim, amb_dim, m_config.TPE.q, m_config.TPE.p);
            if (!m_energyObj || !m_metricObj) {
                throw std::runtime_error("Factory returned nullptr for energy/metric object.");
            }
            m_current_p = m_config.TPE.p;
            m_current_q = m_config.TPE.q;
        } catch (const std::exception& e) {
            polyscope::error("Failed to update energy/metric objects: " + std::string(e.what()));
            m_energyObj = nullptr;
            m_metricObj = nullptr;
            m_current_p = -1.0;
            m_current_q = -1.0;
            throw;
        }
    }
}

bool RepulsorEngine::InitializeRepulsorMesh(SceneObject& object) {
    if (!object.IsSimulated()) {
        return false;
    }
    if (object.GetRepulsorMesh()) {
        polyscope::warning("RepulsorEngine: Mesh already initialized for " + object.GetUniqueName());
        return true;
    }

    const auto& vertices = object.GetInitialVertices();
    const auto& simplices = object.GetSimplices();

    if (vertices.empty() || simplices.empty()) {
        polyscope::error("RepulsorEngine: Cannot initialize mesh for " + object.GetUniqueName() + " - empty geometry.");
        return false;
    }

    Repulsor::SimplicialMesh_Factory<Mesh_T, dom_dim, dom_dim, amb_dim, amb_dim> meshFactory;
    const double (*v_ptr)[amb_dim] = reinterpret_cast<const double (*)[amb_dim]>(vertices.data());
    const int (*s_ptr)[dom_dim + 1] = reinterpret_cast<const int (*)[dom_dim + 1]>(simplices.data());

    try {
        auto meshPtr = meshFactory.Make(v_ptr[0], vertices.size(), amb_dim, false, s_ptr[0], simplices.size(),
                                        dom_dim + 1, false, m_config.TPE.threadCount);

        if (!meshPtr) {
            throw std::runtime_error("MeshFactory::Make returned nullptr.");
        }

        UpdateMeshParametersInternal(meshPtr.get());
        object.SetRepulsorMesh(std::move(meshPtr));

    } catch (const std::exception& e) {
        polyscope::error("RepulsorEngine: Failed to create mesh for " + object.GetUniqueName() + ": " +
                         std::string(e.what()));
        object.SetRepulsorMesh(nullptr);
        return false;
    }

    return true;
}

bool RepulsorEngine::UpdateRepulsorMeshState(SceneObject& object) {
    Mesh_T* meshPtr = object.GetRepulsorMesh();
    if (!meshPtr) {
        polyscope::warning("RepulsorEngine: Cannot update state for " + object.GetUniqueName() + ", no mesh.");
        return false;
    }
    if (!object.IsSimulated()) {
        return true;  // Nothing to update
    }

    // Calculate current world coordinates
    const auto& localVerts = object.GetInitialVertices();
    const auto& transform = object.GetCurrentTransform();
    std::vector<std::array<Real, amb_dim>> worldVerts = Utils::applyTransform(localVerts, transform);

    if (worldVerts.empty()) {
        polyscope::warning("RepulsorEngine: Calculated world vertices are empty for " + object.GetUniqueName());
        return false;
    }

    Tensors::Tensor2<Real, Int> worldTensor = Utils::vecArrayToTensor(worldVerts);
    if (worldTensor.Dimension(0) == 0) {
        return false;
    }

    try {
        meshPtr->ClearCache();
        meshPtr->SemiStaticUpdate(worldTensor.data());
        polyscope::info("Repulsor state updated for " + object.GetUniqueName());
        return true;
    } catch (const std::exception& e) {
        polyscope::error("RepulsorEngine: SemiStaticUpdate failed for " + object.GetUniqueName() + ": " +
                         std::string(e.what()));
        return false;
    }
}

std::unique_ptr<Mesh_T> RepulsorEngine::CreateObstacleMesh(const std::vector<std::array<Real, 3>>& vertices,
                                                           const std::vector<std::array<Int, 3>>& simplices) {
    if (vertices.empty() || simplices.empty()) {
        polyscope::warning("RepulsorEngine::CreateObstacleMesh: Cannot create mesh from empty geometry.");
        return nullptr;
    }

    Repulsor::SimplicialMesh_Factory<Mesh_T, dom_dim, dom_dim, amb_dim, amb_dim> meshFactory;
    const double (*v_ptr)[amb_dim] = reinterpret_cast<const double (*)[amb_dim]>(vertices.data());
    const int (*s_ptr)[dom_dim + 1] = reinterpret_cast<const int (*)[dom_dim + 1]>(simplices.data());

    try {
        auto obstacleMeshPtr = meshFactory.Make(v_ptr[0], vertices.size(), amb_dim, false, s_ptr[0], simplices.size(),
                                                dom_dim + 1, false, m_config.TPE.threadCount);

        if (!obstacleMeshPtr) {
            throw std::runtime_error("Obstacle MeshFactory::Make returned nullptr.");
        }

        UpdateMeshParametersInternal(obstacleMeshPtr.get());
        return obstacleMeshPtr;

    } catch (const std::exception& e) {
        polyscope::error("RepulsorEngine: Failed to create obstacle mesh: " + std::string(e.what()));
        return nullptr;
    }
}

Tensors::Tensor2<Real, Int> RepulsorEngine::CalculateWorldDisplacement(SceneObject& object) {
    Mesh_T* meshPtr = object.GetRepulsorMesh();
    std::lock_guard<std::mutex> lock(m_energyMetricMutex);
    if (!m_energyObj || !m_metricObj) {
        polyscope::error("RepulsorEngine: Energy/Metric objects not available for calculation.");
        throw std::runtime_error("Energy/Metric objects not initialized.");
    }
    if (!meshPtr || !object.IsSimulated()) {
        return Tensors::Tensor2<Real, Int>(0, amb_dim);
    }
    if (meshPtr->VertexCount() == 0) {
        polyscope::warning("RepulsorEngine: Cannot calculate displacement for " + object.GetUniqueName() +
                           ", vertex count is zero.");
        return Tensors::Tensor2<Real, Int>(0, amb_dim);
    }

    try {
        meshPtr->ClearCache();

        Tensors::Tensor2<Real, Int> diff = m_energyObj->Differential(*meshPtr);
        Tensors::Tensor2<Real, Int> gradient(meshPtr->VertexCount(), amb_dim);

        const int max_iter = 100;
        const double relative_tolerance = 1e-5;
        const Int nrhs = diff.Dimension(1);
        if (nrhs != amb_dim) {
            throw std::runtime_error("Differential dimension mismatch.");
        }

        m_metricObj->Solve(*meshPtr, 1.0, diff.data(), nrhs, 0.0, gradient.data(), nrhs, nrhs, max_iter,
                           relative_tolerance);

        Tensors::Tensor2<Real, Int> downward_gradient = gradient;
        downward_gradient *= static_cast<Real>(-1.0);

        double t = meshPtr->MaximumSafeStepSize(downward_gradient.data(), 1.0);

        Tensors::Tensor2<Real, Int> world_displacement = downward_gradient;
        world_displacement *= static_cast<Real>(t);
        return world_displacement;

    } catch (const std::exception& e) {
        polyscope::error("RepulsorEngine: Error calculating displacement for " + object.GetUniqueName() + ": " +
                         std::string(e.what()));
        throw;
    }
}

Real RepulsorEngine::GetEnergy(SceneObject& object) {
    Mesh_T* meshPtr = object.GetRepulsorMesh();
    std::lock_guard<std::mutex> lock(m_energyMetricMutex);
    if (!m_energyObj) {
        polyscope::error("RepulsorEngine: Energy object not available for GetEnergy.");
        throw std::runtime_error("Energy object not initialized.");
    }
    if (!meshPtr || !object.IsSimulated() || meshPtr->VertexCount() == 0) {
        return 0.0;
    }
    try {
        meshPtr->ClearCache();
        return m_energyObj->Value(*meshPtr);
    } catch (...) {
        return 0.0;
    }
}

void RepulsorEngine::UpdateEngineParameters() {
    // TODO: Reverify this logic
    CreateOrUpdateEnergyMetricObjects();
}

void RepulsorEngine::UpdateMeshParametersInternal(Mesh_T* meshPtr) {
    if (!meshPtr) {
        return;
    }
    meshPtr->cluster_tree_settings.split_threshold = m_config.TPE.clusterSplitThreshold;
    meshPtr->cluster_tree_settings.parallel_perc_depth = m_config.TPE.parallelPercolationDepth;
    meshPtr->block_cluster_tree_settings.far_field_separation_parameter = m_config.TPE.farFieldSeparation;
    meshPtr->block_cluster_tree_settings.near_field_separation_parameter = m_config.TPE.nearFieldSeparation;
    meshPtr->block_cluster_tree_settings.near_field_intersection_parameter = m_config.TPE.nearFieldIntersection;
    meshPtr->adaptivity_settings.max_refinement = m_config.TPE.maxRefinement;
    meshPtr->adaptivity_settings.theta = m_config.TPE.theta;
    meshPtr->adaptivity_settings.intersection_theta = m_config.TPE.intersection_theta;
}

void RepulsorEngine::ApplyCurrentConfigToMesh(SceneObject& object) {
    Mesh_T* meshPtr = object.GetRepulsorMesh();
    if (meshPtr) {
        polyscope::info("RepulsorEngine: Applying config to mesh " + object.GetUniqueName());
        UpdateMeshParametersInternal(meshPtr);
    }
}

Tensors::Tensor2<Real, Int> RepulsorEngine::GetDifferential(SceneObject& object) {
    Mesh_T* meshPtr = object.GetRepulsorMesh();
    if (!m_energyObj) {
        throw std::runtime_error("Energy object not initialized.");
    }
    if (!meshPtr || !object.IsSimulated() || meshPtr->VertexCount() == 0) {
        return Tensors::Tensor2<Real, Int>(0, amb_dim);
    }

    try {
        meshPtr->ClearCache();
        return m_energyObj->Differential(*meshPtr);
    } catch (const std::exception& e) {
        polyscope::error("RepulsorEngine: Error getting differential for " + object.GetUniqueName() + ": " +
                         std::string(e.what()));
        throw;
    }
}

Tensors::Tensor2<Real, Int> RepulsorEngine::GetGradient(SceneObject& object) {
    Mesh_T* meshPtr = object.GetRepulsorMesh();
    if (!m_energyObj || !m_metricObj) {
        throw std::runtime_error("Energy/Metric object not initialized.");
    }
    if (!meshPtr || !object.IsSimulated() || meshPtr->VertexCount() == 0) {
        return Tensors::Tensor2<Real, Int>(0, amb_dim);
    }

    try {
        meshPtr->ClearCache();
        Tensors::Tensor2<Real, Int> diff = m_energyObj->Differential(*meshPtr);
        Tensors::Tensor2<Real, Int> gradient(meshPtr->VertexCount(), amb_dim);
        const int max_iter = 100;
        const double relative_tolerance = 1e-5;
        const Int nrhs = diff.Dimension(1);
        if (nrhs != amb_dim) {
            throw std::runtime_error("Differential dimension mismatch.");
        }
        m_metricObj->Solve(*meshPtr, 1.0, diff.data(), nrhs, 0.0, gradient.data(), nrhs, nrhs, max_iter,
                           relative_tolerance);
        return gradient;
    } catch (const std::exception& e) {
        polyscope::error("RepulsorEngine: Error getting gradient for " + object.GetUniqueName() + ": " +
                         std::string(e.what()));
        throw;
    }
}
