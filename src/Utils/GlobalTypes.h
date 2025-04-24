#ifndef GLOBAL_TYPES_H
#define GLOBAL_TYPES_H

#include "Repulsor/submodules/Tensors/MKL.hpp"
#include "Repulsor/Repulsor.hpp"

#include <cstddef>

// Basic Types
using Int = int;
using LInt = std::size_t;
using Real = double;

// Repulsor Types
using Mesh_T = Repulsor::SimplicialMeshBase<Real, Int, LInt>;
using Energy_T = Repulsor::EnergyBase<Mesh_T>;
using Metric_T = Repulsor::MetricBase<Mesh_T>;
using TPE_Factory_T = Repulsor::TangentPointObstacleEnergy_Factory<Mesh_T,2,2,2,2,3,3>;
using TPM_Factory_T = Repulsor::TangentPointMetric0_Factory<Mesh_T, 2, 2, 3, 3>;

// Dimensions
constexpr Int amb_dim = 3;
constexpr Int dom_dim = 2;

#endif // GLOBAL_TYPES_H