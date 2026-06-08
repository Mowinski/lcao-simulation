cmake_minimum_required(VERSION 3.10)
project(HydrogenSimulation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(Freetype REQUIRED)

# OpenCL support
if(APPLE)
    find_library(OPENCL_LIBRARIES OpenCL)
    set(OPENCL_INCLUDE_DIRS "/System/Library/Frameworks/OpenCL.framework/Headers")
    set(OPENCL_FOUND TRUE)
else()
    find_package(OpenCL REQUIRED)
endif()

# ImGui Source Files
set(IMGUI_DIR src/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/Nucleus.cpp 
    src/ElectronCloud.cpp 
    src/TextRenderer.cpp
    src/OpenCLManager.cpp
    ${IMGUI_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    glfw
    GLEW::GLEW
    glm::glm
    Freetype::Freetype
    ${OPENCL_LIBRARIES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    include 
    ${FREETYPE_INCLUDE_DIRS} 
    ${IMGUI_DIR}
    ${OPENCL_INCLUDE_DIRS}
)

# Copy shaders to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)
