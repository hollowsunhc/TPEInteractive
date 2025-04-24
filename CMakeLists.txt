cmake_minimum_required(VERSION 3.21)
project(TPEInteractive VERSION 1.0.0 LANGUAGES CXX)


# --- Policy Flags ---
cmake_policy(SET CMP0092 NEW) # Initialize CMAKE_<LANG>_FLAGS_<CONFIG> to empty.


# --- C++ Standards ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# --- Compiler/Platform Specific Settings ---
if(MSVC)
    # Flags specific to MSVC compiler (cl.exe)
    # Requires Testing
    message(STATUS "Configuring for MSVC")

    # There's an issue where in Repulsor, calling SemiStaticUpdate would segfault with /MDd
    # due to overlapping ranges when swapping. Forcing /MD.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

    set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3 /GR /EHsc")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(WIN32 AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")

        # Flags for clang-cl on Windows (mimicking MSVC)
        message(STATUS "Configuring for clang-cl (Windows)")
        
        # There's an issue where in Repulsor, calling SemiStaticUpdate would segfault with /MDd
        # due to overlapping ranges when swapping. Forcing /MD.
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
        
        set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3 /GR /EHsc")
        set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
        set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL")

    else()
        # Flags for Clang on Linux/macOS/other
        # Requires Testing
        message(STATUS "Configuring for Clang (non-Windows)")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-parameter")
        set(CMAKE_CXX_FLAGS_DEBUG " -O0 -g")
        set(CMAKE_CXX_FLAGS_RELEASE " -O3 -DNDEBUG")
    endif()

elseif(CMAKE_COMPILER_IS_GNUCXX)
    # Flags for GCC on Linux/other
    # Requires Testing
    message(STATUS "Configuring for GCC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_DEBUG " -O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE " -O3 -DNDEBUG")

else()
    message(WARNING "Compiler not explicitly supported. Using default flags.")
endif()


# --- Find Dependencies ---
### BLAS/LAPACK ###
set(BLAS_LAPACK_INCLUDE_DIRS "")
set(BLAS_LAPACK_LIBRARIES "")
set(BLAS_LAPACK_DEFINES "")

if(TPE_BLAS_LAPACK_BACKEND STREQUAL "MKL")
    message(STATUS "Configuring for MKL backend...")
    set(MKL_INTERFACE "ilp64" CACHE STRING "MKL interface type" FORCE)
    find_package(MKL CONFIG REQUIRED)
    
    if(NOT MKL_FOUND)
        message(FATAL_ERROR "MKL not found. Please set MKL_DIR or ensure MKL is discoverable.")
    endif()
    message(STATUS "Found MKL: ${MKL_LIBRARIES}")

    list(APPEND BLAS_LAPACK_LIBRARIES MKL::MKL)
    
    if(MKL_INTERFACE STREQUAL "ilp64")
        list(APPEND BLAS_LAPACK_DEFINES TPE_MKL_USE_ILP64)
    endif()
    list(APPEND BLAS_LAPACK_DEFINES TPE_USE_MKL)


elseif(TPE_BLAS_LAPACK_BACKEND STREQUAL "OpenBLAS")
    message(STATUS "Configuring for OpenBLAS backend...")
    find_package(OpenBLAS CONFIG REQUIRED)
    list(APPEND BLAS_LAPACK_INCLUDE_DIRS ${OpenBLAS_INCLUDE_DIRS})

    # OpenBLAS weirly defines OpenBLAS_LIBRARIES as the DLL path.
    # Based on binary packages located at http://www.openmathlib.org/OpenBLAS/
    set(OPENBLAS_IMPORT_LIB "${_OpenBLAS_ROOT_DIR}/lib/libopenblas.lib")
    if(EXISTS "${OPENBLAS_IMPORT_LIB}")
        list(APPEND BLAS_LAPACK_LIBRARIES ${OPENBLAS_IMPORT_LIB})
    else()
        message(FATAL_ERROR "Manual OpenBLAS import lib not found: ${OPENBLAS_IMPORT_LIB}")
    endif()
    list(APPEND BLAS_LAPACK_DEFINES TPE_USE_OPENBLAS)


elseif(TPE_BLAS_LAPACK_BACKEND STREQUAL "Accelerate")
    message(STATUS "Configuring for Accelerate backend...")
    if(NOT APPLE)
        message(FATAL_ERROR "Accelerate backend is only available on Apple platforms.")
    endif()

    list(APPEND BLAS_LAPACK_LIBRARIES "-framework Accelerate")
    list(APPEND BLAS_LAPACK_DEFINES TPE_USE_ACCELERATE)
    list(APPEND BLAS_LAPACK_DEFINES ACCELERATE_NEW_LAPACK=1)


else()
    message(FATAL_ERROR "Unsupported TPE_BLAS_LAPACK_BACKEND: ${TPE_BLAS_LAPACK_BACKEND}")
endif()


### Polyscope ###
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Use GLM as header-only" FORCE)
set(POLYSCOPE_BUILD_RENDERER_OPENGL_GLFW ON CACHE BOOL "" FORCE)
# set(POLYSCOPE_USE_STANDALONE_GLM OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/polyscope)

if(NOT TARGET polyscope)
    message(FATAL_ERROR "Failed to configure Polyscope.")
endif()
message(STATUS "Configured Polyscope")


### Repulsor ###
add_library(Repulsor::Repulsor INTERFACE IMPORTED)

target_include_directories(Repulsor::Repulsor INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Repulsor
    ${BLAS_LAPACK_INCLUDE_DIRS}
)

target_link_libraries(Repulsor::Repulsor INTERFACE
    ${BLAS_LAPACK_LIBRARIES}
)

target_compile_definitions(Repulsor::Repulsor INTERFACE ${BLAS_LAPACK_DEFINES})
target_compile_options(Repulsor::Repulsor INTERFACE /clang:-fenable-matrix)

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
    src/Utils/BLASLAPACK_Types.h
    src/Utils/GlobalTypes.h
    src/Utils/Helpers.cpp
    src/Utils/Helpers.h
)


# --- Include Directories ---
target_include_directories(TPEInteractiveApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


# --- Linking ---
target_link_libraries(TPEInteractiveApp PRIVATE
    Repulsor::Repulsor
    polyscope
    ${BLAS_LAPACK_LIBRARIES}
)


# --- Compile Definitions ---
target_compile_definitions(TPEInteractiveApp PRIVATE ${BLAS_LAPACK_DEFINES})


# --- Install App ---
install(TARGETS TPEInteractiveApp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
)


# --- Install Dependencies ---
include(InstallRequiredSystemLibraries)
include(GNUInstallDirs)

if(TPE_BLAS_LAPACK_BACKEND STREQUAL "MKL")
    message(STATUS "Adding MKL DLLs to installation...")

    if (NOT DEFINED MKL_DLL_DIR)
        set(MKL_CORE_DLL_FILENAME "mkl_core.2.dll")
        set(MKL_SEARCH_PATHS
            "${MKL_ROOT}/bin/intel64"
            "${MKL_ROOT}/bin"
            "${MKL_ROOT}/redist/intel64/mkl"
            "${MKL_ROOT}/redist/intel64"
        )
        find_file(MKL_CORE_DLL_PATH
            NAMES ${MKL_CORE_DLL_FILENAME}
            PATHS ${MKL_SEARCH_PATHS}
            NO_DEFAULT_PATH # Don't search default system paths
            DOC "Path to the MKL Core DLL (${MKL_CORE_DLL_FILENAME})"
        )

        if(NOT MKL_CORE_DLL_PATH)
            message(FATAL_ERROR "Could not find required MKL DLL (${MKL_CORE_DLL_FILENAME}). Searched in: ${MKL_SEARCH_PATHS}. Please check MKL installation or adjust search paths.")
        endif()

        get_filename_component(MKL_DLL_DIR ${MKL_CORE_DLL_PATH} DIRECTORY)
        message(STATUS "Found MKL runtime directory: ${MKL_DLL_DIR}")
    endif()
    message(STATUS "Attempting to install MKL DLLs from: ${MKL_DLL_DIR}")

    set(MKL_RUNTIME_DLLS
        "mkl_core.2.dll"
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

    install(FILES ${MKL_RUNTIME_DLL_PATHS}
        DESTINATION bin
        COMPONENT Runtime
    )
    # Exclude these from automatic scanner if using it
    set(AUTO_EXCLUDE_REGEXES ${AUTO_EXCLUDE_REGEXES} "${MKL_DLL_DIR}/.*\\.dll")


elseif(TPE_BLAS_LAPACK_BACKEND STREQUAL "OpenBLAS")
    message(STATUS "Adding OpenBLAS DLL to installation...")
    
    # OpenBLAS weirly defines OpenBLAS_LIBRARIES as the DLL path.
    # Based on binary packages located at http://www.openmathlib.org/OpenBLAS/
    set(OpenBLAS_DLL_DIR ${_OpenBLAS_ROOT_DIR}/bin/)
    set(OpenBLAS_DLL_PATH ${OpenBLAS_DLL_DIR}/libopenblas.dll)
    install(FILES ${OpenBLAS_DLL_PATH}
        DESTINATION bin
        COMPONENT Runtime
    )
    set(AUTO_EXCLUDE_REGEXES ${AUTO_EXCLUDE_REGEXES} "${OpenBLAS_DLL_DIR}/.*\\.dll")


elseif(TPE_BLAS_LAPACK_BACKEND STREQUAL "Accelerate")
    message(STATUS "No DLLs to install for Accelerate framework.")
endif()

install(RUNTIME_DEPENDENCY_SET TPEInteractiveApp_deps
    POST_EXCLUDE_REGEXES "^api-ms-win-.*\\.dll$" "^ext-ms-win-.*\\.dll$" ${AUTO_EXCLUDE_REGEXES}
    DESTINATION bin
    COMPONENT Runtime
    DIRECTORIES ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
)

message(STATUS "Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Install target 'TPEInteractiveApp' and its dependencies by running 'cmake --install build --prefix path/to/install'")

message(STATUS "CMake configuration finished for TPEInteractive.")
