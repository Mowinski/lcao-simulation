#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "Nucleus.hpp"
#include "ElectronCloud.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "TextRenderer.hpp"
#include "OpenCLManager.hpp"

// Global camera and mouse state
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f), 5.0f);
float lastX = 400, lastY = 300;
bool firstMouse = true;
bool mouseCaptured = true;
int windowWidth = 800;
int windowHeight = 600;

// Orbital state
OrbitalType orbital1 = OrbitalType::ORBITAL_1S;
OrbitalType orbital2 = OrbitalType::ORBITAL_2PZ;
float mixFactor = 0.0f;
float nucleiDistance = 1.6f;
int targetPointCount = 50000;
float samplingRange = 15.0f;
bool isHybrid = false;
bool isBonding = true;
bool orbitalChanged = false;
float visualRadius = 1.0f;

const char* orbitalNames[] = { "1s", "2s", "2px", "2py", "2pz", "3dz2", "3dxy", "3dyz", "3dxz", "3dx2y2" };

std::string getOrbitalName(OrbitalType type) {
    return orbitalNames[static_cast<int>(type)];
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos; lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos; lastY = ypos;
    if (mouseCaptured)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        mouseCaptured = !mouseCaptured;
        if (mouseCaptured)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        int next = (static_cast<int>(orbital1) + 1) % 10;
        orbital1 = static_cast<OrbitalType>(next);
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        int next = (static_cast<int>(orbital2) + 1) % 10;
        orbital2 = static_cast<OrbitalType>(next);
    }
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Hydrogen Atom Simulation", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    OpenCLManager clManager;
    if (!clManager.init("shaders/simulation.cl")) {
        std::cerr << "OpenCL Initialization failed. Ensure your GPU supports OpenCL." << std::endl;
        return -1;
    }

    TextRenderer textRenderer(800, 600);
    textRenderer.Load("/System/Library/Fonts/Supplemental/Arial.ttf", 24);

    Shader nucleusShader("shaders/nucleus.vert", "shaders/nucleus.frag");
    Shader cloudShader("shaders/cloud.vert", "shaders/cloud.frag");
    Shader simpleShader("shaders/simple.vert", "shaders/simple.frag");

    Nucleus nucleus1(0.1f, 32, 32);
    Nucleus nucleus2(0.1f, 32, 32);
    Nucleus visualSphere(1.0f, 24, 24); // Lower resolution for better readability
    nucleus1.setPosition(glm::vec3(-nucleiDistance / 2.0f, 0.0f, 0.0f));
    nucleus2.setPosition(glm::vec3(nucleiDistance / 2.0f, 0.0f, 0.0f));

    ElectronCloud cloud(&clManager, targetPointCount, nucleus1.getPosition(), nucleus2.getPosition(), orbital1, orbital2, isHybrid ? mixFactor : 0.0f, isBonding, samplingRange);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (orbitalChanged) {
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            textRenderer.RenderText("Calculating cloud points (GPU)...", 250.0f, 300.0f, 0.8f, glm::vec3(1.0f, 0.5f, 0.0f));
            glfwSwapBuffers(window);

            cloud.setTargetPointCount(targetPointCount);
            cloud.updatePoints(nucleus1.getPosition(), nucleus2.getPosition(), orbital1, orbital2, isHybrid ? mixFactor : 0.0f, isBonding, samplingRange);
            orbitalChanged = false;
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main Menu Window
        {
            ImGui::Begin("Simulation Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            
            if (ImGui::SliderFloat("Nuclei Distance", &nucleiDistance, 0.2f, 10.0f)) {
                nucleus1.setPosition(glm::vec3(-nucleiDistance / 2.0f, 0.0f, 0.0f));
                nucleus2.setPosition(glm::vec3(nucleiDistance / 2.0f, 0.0f, 0.0f));
            }
            ImGui::SliderInt("Point Count", &targetPointCount, 10000, 1000000);
            ImGui::SliderFloat("Sampling Range", &samplingRange, 2.0f, 40.0f);
            ImGui::Separator();

            int orb1 = static_cast<int>(orbital1);
            ImGui::Combo("Orbital 1", &orb1, orbitalNames, IM_ARRAYSIZE(orbitalNames));
            orbital1 = static_cast<OrbitalType>(orb1);

            ImGui::Checkbox("Hybrid Mode", &isHybrid);
            
            if (isHybrid) {
                int orb2 = static_cast<int>(orbital2);
                ImGui::Combo("Orbital 2", &orb2, orbitalNames, IM_ARRAYSIZE(orbitalNames));
                orbital2 = static_cast<OrbitalType>(orb2);
                ImGui::SliderFloat("Mix Factor", &mixFactor, 0.0f, 1.0f);
            }

            ImGui::Checkbox("Bonding State", &isBonding);
            ImGui::Separator();
            ImGui::SliderFloat("Visual Radius (r)", &visualRadius, 0.1f, samplingRange);

            ImGui::Separator();
            if (ImGui::Button("Calculate Simulation (GPU)", ImVec2(-1, 40))) {
                orbitalChanged = true;
            }

            ImGui::Separator();
            ImGui::Text("Camera Controls:");
            ImGui::Text("- ESC to toggle Mouse Capture");
            ImGui::Text("- Mouse to rotate");
            ImGui::Text("- Scroll to zoom");

            ImGui::End();
        }

        // Chart Window Placeholder (Bottom Right)
        {
            ImGuiIO& io = ImGui::GetIO();
            float windowWidth = 300.0f;
            float windowHeight = 150.0f;
            float margin = 10.0f;
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - windowWidth - margin, io.DisplaySize.y - windowHeight - margin), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

            ImGui::Begin("Analysis Chart", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
            ImGui::Text("Radial Probability Density P(r)");
            
            const std::vector<float>& densityData = cloud.getRadialDensity();
            if (!densityData.empty()) {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                
                ImGui::PlotLines("##density_plot", densityData.data(), static_cast<int>(densityData.size()), 0, nullptr, 0.0f, 1.1f, ImVec2(-1, -1));
                
                // Draw vertical line for visualRadius
                float fraction = visualRadius / samplingRange;
                if (fraction >= 0.0f && fraction <= 1.0f) {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    float lineX = pos.x + fraction * size.x;
                    drawList->AddLine(ImVec2(lineX, pos.y), ImVec2(lineX, pos.y + size.y), IM_COL32(255, 0, 0, 255), 2.0f);
                }
            } else {
                ImGui::Text("No data available. Click 'Calculate'.");
            }
            
            ImGui::End();
        }

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 viewProj = projection * view;

        nucleus1.draw(viewProj, nucleusShader.ID);
        nucleus2.draw(viewProj, nucleusShader.ID);
        
        // Draw visual radius sphere (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glm::mat4 sphereModel = glm::scale(glm::mat4(1.0f), glm::vec3(visualRadius / 1.0f));
        glUseProgram(simpleShader.ID);
        glUniform4f(glGetUniformLocation(simpleShader.ID, "uColor"), 1.0f, 1.0f, 1.0f, 0.2f); // Faint white
        
        visualSphere.draw(viewProj, sphereModel, simpleShader.ID); 
        
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_BLEND);

        glDepthMask(GL_FALSE);
        cloud.draw(viewProj, cloudShader.ID);
        glDepthMask(GL_TRUE);

        // Orbital Information Display
        textRenderer.updateProjection(windowWidth, windowHeight);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE); 
        glDisable(GL_DEPTH_TEST);

        glEnable(GL_DEPTH_TEST);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
