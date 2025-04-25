# Building TPEInteractive

This guide provides detailed instructions for building TPEInteractive from source on different platforms.

## Prerequisites Summary

*   **CMake:** 3.21+
*   **C++20 Compiler:** (clang-cl 19+, GCC 13+, Clang 12+)
*   **Ninja Build System:** (Recommended)
*   **BLAS/LAPACK Backend:**
    *   Intel oneMKL (Recommended for Intel CPUs) OR
    *   OpenBLAS (Good cross-platform option)

## General Build Process (Using CMake Presets)

The recommended way to configure and build is using [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html). This project includes `CMakePresets.json` defining common configurations.

1.  **Clone:**
    ```bash
    git clone --recursive https://your-repo-url/TPEInteractive.git
    cd TPEInteractive
    ```
    *(Ensure submodules in `vendor/` are populated)*

2.  **Select Preset:** Choose a `configurePreset` from `CMakePresets.json` that matches your system and desired BLAS backend (e.g., `native-windows-MKL`, `native-linux-OpenBLAS` - *You'll need to add Linux presets*).

3.  **Configure:**
    ```bash
    # Replace <preset-name> with your choice
    cmake --preset <preset-name>
    ```
    *Example for Windows MKL:*
    ```bash
    cmake --preset native-windows-MKL
    ```
    *Example for Linux OpenBLAS (assuming you add this preset):*
    ```bash
    cmake --preset native-linux-OpenBLAS
    ```
    *Adjust CMake variable paths proper to your setup.*

4.  **Build:** Choose a `buildPreset` (which links to a `configurePreset` and specifies Debug/Release).
    ```bash
    # Replace <build-preset-name> with your choice (e.g., native-MKL-debug)
    cmake --build --preset <build-preset-name>
    ```
    *Example:*
    ```bash
    cmake --build --preset native-MKL-debug
    ```
    *Alternatively, build a specific configuration directly:*
    ```bash
    cmake --build build --config Release # Or Debug
    ```
    *(Replace `build` with your binary directory if different from the preset)*

5.  **Install (Optional but Recommended for Running):** This copies the executable and necessary runtime libraries to a clean location.
    ```bash
    # Replace <install-path> with your desired installation directory
    # Replace <build-preset-name> or specify config manually
    cmake --install build --prefix <install-path> --config Release # Or Debug
    ```
    *Example:*
    ```bash
    cmake --install build --prefix ./dist --config Release
    ```
    The executable will be in `<install-path>/bin`.

    Otherwise, if you are using VSCode and would like to either debug the application or simply run the release, some launch configurations are provided. Make sure to adjust the `PATH` environment variable for the selected BLAS backend (MKL or OpenBLAS).

## Platform Specific Notes

### Windows (Visual Studio / clang-cl)

*   Ensure you have the "Desktop development with C++" workload installed in Visual Studio.
*   Make sure CMake can find your chosen compiler. I recommend launching your IDE from a VS Developer Command Prompt. 
*   **MKL:** Provide `MKL_DIR` to CMake, where `MKLConfig.cmake` is located (usually at `intel-mkl/lib/cmake/mkl`).
*   **OpenBLAS:** Download pre-built binaries (including `include`, `lib`, `bin`) or build from source. Provide `OpenBLAS_DIR` to CMake.
*   The build process creates `TPEInteractive.exe`. The `install` step copies required `.dll` files.

### Linux (GCC / Clang) (*Unverified. Might not be correct.*)

*   Install development tools: `sudo apt update && sudo apt install build-essential cmake ninja-build` (Debian/Ubuntu) or equivalent.
*   **MKL:** Install via Intel installers. Set `MKLROOT` or ensure the MKL CMake config files are findable. You might need to source MKL environment scripts (`source /opt/intel/oneapi/setvars.sh`).
*   **OpenBLAS:** Install development packages: `sudo apt install libopenblas-dev` or equivalent. `find_package(OpenBLAS)` should then work without needing `OpenBLAS_DIR`.
*   The build process creates executable files (no extension). The `install` step copies required `.so` files (shared objects). You might need to configure `RPATH` during the build or set `LD_LIBRARY_PATH` at runtime if libraries are installed in non-standard locations.

    *CMake RPATH settings (add after `add_executable`):*
    ```cmake
    # Set RPATH so executable finds libraries in install directory
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}") # For libraries installed by this project
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) # Also add directories of linked libraries found during build
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) # Embed RPATH during build (useful for running from build dir)
    ```

### macOS (Clang / Accelerate)  (*Unverified. Might not be correct.*)

*   Install Xcode and Command Line Tools.
*   Install CMake and Ninja (e.g., via Homebrew: `brew install cmake ninja`).
*   **Accelerate:** This is part of macOS, just select the `Accelerate` backend in CMake (`-DTPE_BLAS_LAPACK_BACKEND=Accelerate`). No extra libraries needed.
*   **MKL/OpenBLAS:** Can also be installed via Homebrew or Intel/OpenBLAS websites. Configure CMake similarly to Linux.

## Troubleshooting

*   **Dependency Not Found:** Double-check installation paths and ensure CMake cache variables (`MKL_DIR`, `OpenBLAS_DIR`) are set correctly. Delete `CMakeCache.txt` in the build directory and re-configure.
*   **Runtime Errors (Missing DLL/SO):** Use the `install` step to gather dependencies or manually set `PATH` (Windows) or `LD_LIBRARY_PATH` (Linux) to include the directories containing the required runtime libraries. Consider setting `RPATH` on Linux/macOS builds.

Please report persistent build issues on the [GitHub Issues](https://your-repo-url/TPEInteractive/issues) page, providing details about your OS, compiler, CMake version, and the specific error messages.
