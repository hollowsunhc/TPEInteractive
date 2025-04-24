# TPEInteractive

Interactive Example for the Tangent Point Energy

# Build Stage

## Requirements

- Intel MKL

Other dependencies are vendored.

## Windows

This app has only been tested on Windows with the clang-cl compiler. Download the latest clang-cl here: https://github.com/llvm/llvm-project/releases

Using CMake, configure with a provided MKL_DIR pointing to its directory containing `MKLConfig.cmake`.

I recommend editing the `native-windows` configure preset in the `CMakePresets.json` and configure by running the command `cmake --preset native-windows`.

Since VSCode is my editor of choice, most of the environment will automatically be setup for use, minus path adjustments for Intel MKL in `CMakePresets.json` and `.vscode/launch.json`.
