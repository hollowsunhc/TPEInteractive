# TPEInteractive Included Examples

This document describes the example scenes included with TPEInteractive.

## Loading Examples

Use the "Select Example" dropdown in the UI panel to choose and load an example. Loading a new example will reset the current state.

## 1. FCC 4 Spheres

*   **File:** Defined in `src/Examples/ExampleLoader.cpp::CreateFCC4SphereScene()`
*   **Description:** Creates a small face-centered cubic (FCC) lattice arrangement of four identical spheres within a defined region.
*   **Objects:**
    *   `sphere_0` to `sphere_3`:
        *   `isSimulated`: True
        *   `isInteractive`: True
        *   `isObstacleSource`: True
        *   `Obstacle Definition`: All *other* spheres act as obstacles (`{-1}`).
*   **Purpose:** Demonstrates mutual interaction between multiple simulated objects where each acts as an obstacle for the others. Useful for testing basic TPE forces and gradient descent steps in a multi-body scenario.

## 2. Two Spheres

*   **File:** Defined in `src/Examples/ExampleLoader.cpp::CreateTwoSphereScene()`
*   **Description:** A simple scene with one interactive sphere and one static obstacle sphere. Based on the data provided with the Repulsor library.
*   **Objects:**
    *   `sphere_main_0`:
        *   `isSimulated`: True (Its position is updated by physics)
        *   `isInteractive`: True (Can be moved with the gizmo)
        *   `isObstacleSource`: False (Its shape does not act as a barrier)
        *   `Obstacle Definition`: Object with ID 1 (`{1}`).
    *   `sphere_obstacle_1`:
        *   `isSimulated`: False (Its position is fixed)
        *   `isInteractive`: False (Cannot be moved with the gizmo)
        *   `isObstacleSource`: True (Its shape *is* the obstacle for `sphere_main_0`)
        *   `Obstacle Definition`: None (`{}`).
*   **Purpose:** Demonstrates the interaction of a simulated object with a fixed obstacle. Useful for testing the obstacle loading and interaction parts of the Repulsor library and verifying energy/gradients relative to a static barrier.

*(Add details for any other examples you create)*
