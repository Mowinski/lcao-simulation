# Hydrogen Atom Simulation (H2/H2+)

This project focuses on the numerical simulation and visualization of the hydrogen molecule (H2) or its ion (H2+). The primary goal is to visualize the electron probability density in 3D using OpenGL.

## Tech Stack
- **Language:** C++20
- **Graphics API:** OpenGL 4.5+
- **Window Management:** GLFW
- **OpenGL Loading:** glad (or GLEW)
- **Math:** GLM
- **Build System:** CMake

## Project Structure
- `src/`: Source files (.cpp)
- `include/`: Header files (.h, .hpp)
- `shaders/`: GLSL shader files
- `build/`: CMake build directory (ignored by git)

## Conventions
- Use modern OpenGL (Core Profile).
- Use GLM for all linear algebra.
- Follow a clean, object-oriented approach for simulation vs. visualization.
- Document simulation parameters (e.g., nuclear distance for H2+).

## Workflows
- Always verify simulation results against theoretical values when possible.
- Use `CMake` to manage dependencies and build process.
