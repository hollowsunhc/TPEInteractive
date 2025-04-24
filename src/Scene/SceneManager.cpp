#include "SceneManager.h"
#include "SceneObject.h"
#include "../Engine/RepulsorEngine.h"
#include "../Engine/VisualizationEngine.h"
#include "../Config/Config.h"
#include "../Utils/Helpers.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <polyscope/polyscope.h>


SceneManager::SceneManager(RepulsorEngine& repulsorEngine, VisualizationEngine& vizEngine, const ConfigType& config)
    : m_repulsorEngine(repulsorEngine), m_vizEngine(vizEngine), m_config(config) {}


Utils::CombinedObstacleGeometry SceneManager::CalculateCombinedObstacleGeometryForObject(int targetObjectId) {
    Utils::CombinedObstacleGeometry result;
    if (!m_currentSceneDef) {
        polyscope::error("CalcCombineObstacle: No scene loaded.");
        return result;
    }

    // Find Target SceneObjectDefinition
    const SceneObjectDefinition* targetObjDefPtr = nullptr;
    for (const auto& def : m_currentSceneDef->objectDefs) {
        if (def.id == targetObjectId) {
            targetObjDefPtr = &def;
            break;
        }
    }
    if (!targetObjDefPtr) {
        polyscope::warning("CalcCombineObstacle: Could not find SceneObjectDefinition for target ID " + std::to_string(targetObjectId));
        return result;
    }
    const auto& targetObjDef = *targetObjDefPtr;

    // Determine Source SceneObjectDefinitions
    std::vector<const SceneObjectDefinition*> sourceDefs;
    if (targetObjDef.obstacleDefinitionIds.empty()) {
        result.success = true; // No obstacle defined is a success (empty result)
        return result;
    }

    if (targetObjDef.obstacleDefinitionIds.size() == 1 && targetObjDef.obstacleDefinitionIds[0] == -1) {
        // Combine all OTHER obstacle sources
        for (const auto& sourceDef : m_currentSceneDef->objectDefs) {
            if (sourceDef.id != targetObjDef.id && sourceDef.isObstacleSource) {
                sourceDefs.push_back(&sourceDef);
            }
        }
    } else {
        // Combine specific objects by ID
        std::map<int, const SceneObjectDefinition*> sourceMap;
        for (const auto& def : m_currentSceneDef->objectDefs) {
            if (def.isObstacleSource) sourceMap[def.id] = &def;
        }
        for (int source_id : targetObjDef.obstacleDefinitionIds) {
            if (source_id != targetObjDef.id && sourceMap.count(source_id)) {
                sourceDefs.push_back(sourceMap.at(source_id));
            }
        }
    }

    if (sourceDefs.empty()) {
        result.success = true; // No valid sources found is a success (empty result)
        return result;
    }

    // Combine Source Geometries Using Current World Coordinates from runtime state (m_objects)
    int current_vertex_offset = 0;
    for (const auto* sourceDefPtr : sourceDefs) {
        const auto& sourceDef = *sourceDefPtr;

        // Find the corresponding runtime SceneObject for this source definition
        SceneObject* sourceRuntimeObj = GetObjectById(sourceDef.id); // Use helper
        if (!sourceRuntimeObj) {
            polyscope::warning("CalcCombineObstacle: Could not find Runtime Object for source ID " + std::to_string(sourceDef.id));
            continue; // Skip this source
        }

        // Get source's INITIAL vertices, CURRENT transform, and simplices definition
        const auto& sourceInitialVertices = sourceRuntimeObj->GetInitialVertices(); // Get from runtime obj
        const auto& sourceTransform = sourceRuntimeObj->GetCurrentTransform();       // Get from runtime obj
        const auto* sourceSimplicesPtr = sourceDef.meshData ? &sourceDef.meshData->simplices : nullptr; // Get from definition

        if (sourceInitialVertices.empty() || !sourceSimplicesPtr || sourceSimplicesPtr->empty()) {
            polyscope::warning("CalcCombineObstacle: Skipping source " + sourceDef.baseName + std::to_string(sourceDef.id) + " (missing geom).");
            continue; // Skip invalid source
        }
        const auto& sourceSimplices = *sourceSimplicesPtr;

        // Calculate CURRENT WORLD coordinates for this source
        std::vector<std::array<Real, amb_dim>> transformedSourceVerts =
            Utils::applyTransform(sourceInitialVertices, sourceTransform);

        // Append transformed vertices and adjusted simplices
        result.combined_world_vertices.insert(
            result.combined_world_vertices.end(),
            transformedSourceVerts.begin(), transformedSourceVerts.end()
        );
        for (const auto& simplex_orig : sourceSimplices) {
            result.combined_simplices.push_back({
                simplex_orig[0] + current_vertex_offset,
                simplex_orig[1] + current_vertex_offset,
                simplex_orig[2] + current_vertex_offset
            });
        }
        current_vertex_offset += transformedSourceVerts.size();
    } // End loop over sources

    result.success = true;
    return result;
}


void SceneManager::UpdateRepulsorObstacleForObject(SceneObject& targetObject, const Utils::CombinedObstacleGeometry& obsGeo) {
    Mesh_T* targetMesh = targetObject.GetRepulsorMesh();
    if (!targetMesh) {
        polyscope::warning("UpdateRepulsorObstacle: Target object " + targetObject.GetUniqueName() + " has no Repulsor mesh.");
        return;
    }
    if (!obsGeo.success) {
        polyscope::error("UpdateRepulsorObstacle: Input geometry calculation failed for " + targetObject.GetUniqueName() + ". Obstacle not updated.");
        return;
    }

    size_t v_count = obsGeo.combined_world_vertices.size();
    size_t f_count = obsGeo.combined_simplices.size();

    std::unique_ptr<Mesh_T> newObstacleMesh = nullptr;

    if (v_count > 0 && f_count > 0) {
        newObstacleMesh = m_repulsorEngine.CreateObstacleMesh(
            obsGeo.combined_world_vertices,
            obsGeo.combined_simplices
        );
        if (!newObstacleMesh) {
            polyscope::error("UpdateRepulsorObstacle: RepulsorEngine failed to create new obstacle mesh for " + targetObject.GetUniqueName() + ". Obstacle not updated.");
            return;
        }

    } else {
        polyscope::info("UpdateRepulsorObstacle: Combined obstacle for " + targetObject.GetUniqueName() + " is empty. No obstacle loaded/cleared.");
    }

    // --- Load the new mesh ---
    if (newObstacleMesh) { // Only attempt load if we have a valid mesh ptr
        try {
            targetMesh->LoadObstacle(std::move(newObstacleMesh));
        } catch (const std::exception& e) {
            polyscope::error("UpdateRepulsorObstacle: Exception during LoadObstacle for " + targetObject.GetUniqueName() + ": " + std::string(e.what()));
        }
    } else {
        polyscope::info("UpdateRepulsorObstacle: No valid new obstacle mesh created/provided for " + targetObject.GetUniqueName() + ". Existing obstacle remains unchanged.");
    }
}


void SceneManager::UpdateObstaclesForAllObjects() {
    if (!m_currentSceneDef) return;

    polyscope::info("Updating obstacles for all relevant objects...");
    std::vector<int> updated_object_ids;

    for (auto& objPtr : m_objects) {
        if (!objPtr->IsSimulated()) {
            continue;
        }

        Utils::CombinedObstacleGeometry obsGeo = CalculateCombinedObstacleGeometryForObject(objPtr->GetId());

        UpdateRepulsorObstacleForObject(*objPtr, obsGeo);
        updated_object_ids.push_back(objPtr->GetId());
    }

    for (int id : updated_object_ids) {
        SceneObject* obj = GetObjectById(id);
        if (obj) {
            m_vizEngine.UpdateSingleObstacleVisual(*obj);
        }
    }

    polyscope::info("Obstacle updates complete.");
}


bool SceneManager::LoadScene(const SceneDefinition& sceneDef) {
    UnloadScene();
    m_currentSceneDef = std::make_unique<SceneDefinition>(sceneDef);

    polyscope::info("Loading scene: " + m_currentSceneDef->sceneName);

    m_objects.reserve(m_currentSceneDef->objectDefs.size());
    for (const auto& objDef : m_currentSceneDef->objectDefs) {
        m_objects.push_back(std::make_unique<SceneObject>(objDef));

        SceneObject* newObj = m_objects.back().get();

        if (newObj->IsSimulated()) {
            bool meshCreated = m_repulsorEngine.InitializeRepulsorMesh(*newObj);
            if (!meshCreated) {
                polyscope::error("Failed to initialize Repulsor mesh for " + newObj->GetUniqueName());
                UnloadScene();
                return false;
            }
        }

        // Register with visualization
        m_vizEngine.RegisterObject(*newObj);
    }

    UpdateObstaclesForAllObjects();

    // --- Set initial active object ---
    m_activeObjectId = -1;
    std::string activeObjectName = "";
    for (size_t i = 0; i < m_objects.size(); ++i) {
        if (m_objects[i]->IsInteractive()) {
            m_activeObjectId = m_objects[i]->GetId();
            activeObjectName = m_objects[i]->GetUniqueName();
            break;
        }
    }

    // Tell VisualizationEngine about the initial active object (no previous one)
    m_vizEngine.UpdateActiveGizmo("", activeObjectName); // Pass empty string for old name

    polyscope::info("Scene loaded successfully.");
    return true;
}


void SceneManager::UnloadScene() {
    polyscope::info("SceneManager: Unloading current scene...");
    m_vizEngine.RemoveAllObjects();
    m_currentSceneDef.reset();
    m_activeObjectId = -1;
    m_objects.clear();
    polyscope::info("SceneManager: Scene unloaded.");
}


void SceneManager::UpdateObjectTransform(int objectId, const glm::mat4& newTransform) {
    SceneObject* obj = GetObjectById(objectId);
    if (!obj) return;

    if (Utils::matricesAreClose(obj->GetCurrentTransform(), newTransform)) {
        return;
    }

    SynchronizeObjectState(objectId, newTransform);
}


void SceneManager::ApplyPhysicsStep(int iterations) {
    polyscope::info("SceneManager: Applying " + std::to_string(iterations) + " physics step(s)...");
    bool step_ok = true;
    std::map<int, Utils::IterationData> iteration_results;

    for (int iter = 0; iter < iterations && step_ok; ++iter) {
        polyscope::info(" === Physics Step " + std::to_string(iter + 1) + " ===");

        // Calculate and apply updates for one step
        step_ok = CalculateAndApplyPhysicsUpdates(iteration_results);

        if (!step_ok) {
            polyscope::error("Physics step " + std::to_string(iter + 1) + " failed during calculation or application. Stopping iterations.");
            break;
        }

        UpdateObstaclesForAllObjects();

    }

    polyscope::info("Physics step(s) application attempt finished.");
    m_vizEngine.RequestRedraw();
}


void SceneManager::UpdateEngineParametersForAllObjects() {
    polyscope::info("SceneManager: Updating Repulsor parameters from config...");
    m_repulsorEngine.UpdateEngineParameters(); // Handles p/q changes

    for (auto& objPtr : m_objects) {
        if (objPtr->IsSimulated() && objPtr->GetRepulsorMesh()) {
            m_repulsorEngine.ApplyCurrentConfigToMesh(*objPtr);
        }
    }
    polyscope::info("SceneManager: Parameter update request complete.");
}


SceneObject* SceneManager::GetActiveObject() {
    return GetObjectById(m_activeObjectId);
}


bool SceneManager::SetActiveObjectId(int id) {
    SceneObject* newActiveObj = GetObjectById(id);

    // Ensure the new object is interactive before setting
    if (newActiveObj && newActiveObj->IsInteractive()) {
        int oldActiveId = m_activeObjectId;
        SceneObject* oldActiveObj = GetObjectById(oldActiveId);

        m_activeObjectId = id;

        std::string oldName = oldActiveObj ? oldActiveObj->GetUniqueName() : "";
        std::string newName = newActiveObj->GetUniqueName();
        polyscope::info("SceneManager: Calling UpdateActiveGizmo with old='" + oldName + "', new='" + newName + "'");

        m_vizEngine.UpdateActiveGizmo(oldName, newName);

        return true;
    } else if (id == -1 && m_activeObjectId != -1) {
        SceneObject* oldActiveObj = GetObjectById(m_activeObjectId);
        std::string oldName = oldActiveObj ? oldActiveObj->GetUniqueName() : "";
        m_activeObjectId = -1;
        m_vizEngine.UpdateActiveGizmo(oldName, "");
        return true;
    }
    return false;
}


SceneObject* SceneManager::GetObjectById(int id) {
    if (id == -1) return nullptr;
    for (auto& objPtr : m_objects) {
        if (objPtr->GetId() == id) {
            return objPtr.get();
        }
    }
    return nullptr;
}


const std::vector<std::unique_ptr<SceneObject>>& SceneManager::GetObjects() const {
    return m_objects;
}


void SceneManager::SynchronizeObjectState(int objectId, const glm::mat4& newTransform) {
    // NOTE: This function assumes the caller (UpdateObjectTransform) has already
    //       validated the objectId and checked if the transform actually changed.
    SceneObject* obj = GetObjectById(objectId);
    if (!obj) return;

    obj->SetCurrentTransform(newTransform);

    bool synced = m_repulsorEngine.UpdateRepulsorMeshState(*obj);

    if (synced) {
        UpdateObstaclesForAllObjects();
    } else {
        polyscope::error("Failed to sync Repulsor state for " + obj->GetUniqueName() + " after transform update.");
    }
}


bool SceneManager::CalculateAndApplyPhysicsUpdates(std::map<int, Utils::IterationData>& results) {
    bool any_calc_failed = false;
    results.clear();

    // --- Calculate Updates ---
    for (auto& objPtr : m_objects) {
        if (!objPtr->IsSimulated() || !objPtr->GetRepulsorMesh()) continue;

        int id = objPtr->GetId();
        results[id] = Utils::IterationData();

        try {
            results[id].world_displacement = m_repulsorEngine.CalculateWorldDisplacement(*objPtr);
            results[id].updated = true;
        } catch (const std::exception& e) {
            polyscope::error("Physics calc failed for " + objPtr->GetUniqueName() + ": " + e.what());
            results[id].updated = false;
            any_calc_failed = true;
        }
    }

    if (any_calc_failed) {
        polyscope::warning("Aborting physics application due to calculation errors.");
        return false;
    }

    // --- Apply Updates ---
    for (auto& objPtr : m_objects) {
        if (!objPtr->IsSimulated() || !results.count(objPtr->GetId()) || !results[objPtr->GetId()].updated) {
            continue;
        }

        int id = objPtr->GetId();
        const auto& resultData = results[id];

        try {
            glm::mat4 invTransform = glm::affineInverse(objPtr->GetCurrentTransform());
            std::vector<std::array<Real, 3>> local_delta_vec;
            const auto& world_disp = resultData.world_displacement;
            local_delta_vec.reserve(world_disp.Dimension(0));

            for(int v=0; v < world_disp.Dimension(0); ++v) {
                glm::vec4 world_d = {(float)world_disp(v,0), (float)world_disp(v,1), (float)world_disp(v,2), 0.0f};
                glm::vec4 local_d = invTransform * world_d;
                local_delta_vec.push_back({(Real)local_d.x, (Real)local_d.y, (Real)local_d.z});
            }

            objPtr->UpdateInitialVertices(local_delta_vec);

            m_repulsorEngine.UpdateRepulsorMeshState(*objPtr);

            m_vizEngine.UpdateObjectVertices(*objPtr);

        } catch (const std::exception& e) {
            polyscope::error("Failed applying physics update for " + objPtr->GetUniqueName() + ": " + e.what());
            any_calc_failed = true;
        }
    }

    return !any_calc_failed;
}
