cmake_minimum_required(VERSION 3.21)
project(TPEInteractive VERSION 1.0.0 LANGUAGES CXX)

# --- Policy Flags ---
cmake_policy(SET CMP0092 NEW) # Initialize CMAKE_<LANG>_FLAGS_<CONFIG> to empty.

# --- C++ Standards ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- Basic Settings ---
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# --- Compiler/Platform Specific Settings ---
set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3 /GR /EHsc")

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O3 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "/O3 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL")
else()
    message(WARNING "Compiler not explicitly supported. Using default flags.")
endif()

# There's an issue where in Repulsor, calling SemiStaticUpdate would segfault with /MDd
# due to overlapping ranges when swapping. Forcing /MD.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# --- Find Dependencies ---

### MKL ###
set(MKL_INTERFACE "ilp64" CACHE STRING "MKL interface type" FORCE)
find_package(MKL CONFIG REQUIRED)

if(NOT MKL_FOUND)
    message(FATAL_ERROR "MKL not found. Please set MKL_DIR or ensure MKL is discoverable.")
endif()
message(STATUS "Found MKL: ${MKL_LIBRARIES}")

### Polyscope ###
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Use GLM as header-only" FORCE)
# set(POLYSCOPE_BUILD_RENDERER_OPENGL_GLFW ON CACHE BOOL "" FORCE)
# set(POLYSCOPE_USE_STANDALONE_GLM OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/polyscope)

if(NOT TARGET polyscope)
    message(FATAL_ERROR "Failed to configure Polyscope.")
endif()
message(STATUS "Configured Polyscope")

### Repulsor ###
add_library(Repulsor::Repulsor INTERFACE IMPORTED)
target_include_directories(Repulsor::Repulsor INTERFACE vendor/Repulsor)
target_compile_options(Repulsor::Repulsor INTERFACE /clang:-fenable-matrix)

if(MKL_INTERFACE STREQUAL "ilp64")
    message(STATUS "Setting MKL interface to ILP64")
    target_link_libraries(Repulsor::Repulsor INTERFACE
        MKL::mkl_intel_ilp64
        MKL::mkl_core
        MKL::mkl_intel_thread
    )
    target_compile_definitions(Repulsor::Repulsor INTERFACE TPE_MKL_USE_ILP64)
elseif(MKL_INTERFACE STREQUAL "lp64")
    message(STATUS "Setting MKL interface to LP64")
    target_link_libraries(Repulsor::Repulsor INTERFACE
        MKL::mkl_intel_lp64
        MKL::mkl_core
        MKL::mkl_intel_thread
    )
else()
    message(WARNING "Unknown MKL_INTERFACE. Assuming LP64.")
endif()

message(STATUS "Configured Repulsor")

# --- Application Target ---
add_executable(TPEInteractiveApp "")

# --- Add Source Files ---
target_sources(TPEInteractiveApp PRIVATE
    src/main.cpp

    # Application
    src/Application/Application.cpp
    src/Application/Application.h

    # Scene
    src/Scene/SceneManager.cpp
    src/Scene/SceneManager.h
    src/Scene/SceneObject.cpp
    src/Scene/SceneObject.h

    # Engine
    src/Engine/RepulsorEngine.cpp
    src/Engine/RepulsorEngine.h
    src/Engine/VisualizationEngine.cpp
    src/Engine/VisualizationEngine.h

    # UI
    src/UI/UIManager.cpp
    src/UI/UIManager.h

    # Examples
    src/Examples/EmbeddedMeshData.h
    src/Examples/ExampleLoader.cpp
    src/Examples/ExampleLoader.h
    src/Examples/FCCLatticeSpheres.h


    # Data
    src/Data/MeshData.h
    src/Data/SceneDefinition.h

    # Config
    src/Config/Config.h

    # Utils
    src/Utils/Helpers.cpp
    src/Utils/Helpers.h
    src/Utils/GlobalTypes.h
)

# --- Include Directories ---
target_include_directories(TPEInteractiveApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- Linking ---
target_link_libraries(TPEInteractiveApp PRIVATE
    Repulsor::Repulsor
    polyscope
    MKL::MKL
)

# --- Install ---
install(TARGETS TPEInteractiveApp
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if (NOT DEFINED MKL_DLL_DIR)
    set(MKL_DLL_DIR "${MKL_ROOT}/bin") # Or mkl/redist/... depending on layout
endif()
message(STATUS "Attempting to install MKL DLLs from: ${MKL_DLL_DIR}")

# List the known required MKL DLLs (adjust based on ILP64/LP64 and threading)
set(MKL_RUNTIME_DLLS
    "mkl_core.2.dll"            # Or mkl_core.1.dll for older versions
)

set(MKL_RUNTIME_DLL_PATHS "")
foreach(DLL_NAME ${MKL_RUNTIME_DLLS})
    list(APPEND MKL_RUNTIME_DLL_PATHS "${MKL_DLL_DIR}/${DLL_NAME}")
endforeach()

foreach(DLL_PATH ${MKL_RUNTIME_DLL_PATHS})
    if(NOT EXISTS "${DLL_PATH}")
        message(WARNING "MKL DLL not found at expected location: ${DLL_PATH}")
    endif()
endforeach()

# Install each required MKL DLL to the same directory as the executable
install(FILES ${MKL_RUNTIME_DLL_PATHS}
    DESTINATION bin
    COMPONENT Runtime
)

# --- Install Other Dependencies ---
include(InstallRequiredSystemLibraries)
include(GNUInstallDirs)
install(RUNTIME_DEPENDENCY_SET TPEInteractiveApp_deps
    POST_EXCLUDE_REGEXES "^api-ms-win-.*\\.dll$" "^ext-ms-win-.*\\.dll$"
    # Exclude the MKL DLLs we already handled explicitly
    "${MKL_DLL_DIR}/mkl_core\\.2\\.dll"
    "${MKL_DLL_DIR}/mkl_intel_thread\\.2\\.dll"
    "${MKL_DLL_DIR}/mkl_intel_ilp64\\.2\\.dll"
    "${MKL_DLL_DIR}/libiomp5md\\.dll"
    DESTINATION bin
    COMPONENT Runtime
    DIRECTORIES
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
)

message(STATUS "Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Install target 'TPEInteractiveApp' and its dependencies by running 'cmake --install build --prefix path/to/install'")

message(STATUS "CMake configuration finished for TPEInteractive.")
