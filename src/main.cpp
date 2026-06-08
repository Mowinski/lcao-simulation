#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

const char* orbitalNames[] = { "1s", "2s", "2px", "2py", "2pz", "3dz2", "3dxy", "3dyz", "3dxz", "3dx2y2" };

std::string getOrbitalName(OrbitalType type) {
    return orbitalNames[static_cast<int>(type)];
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
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
    
    Nucleus nucleus1(0.1f, 32, 32);
    Nucleus nucleus2(0.1f, 32, 32);
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

            int orb2 = static_cast<int>(orbital2);
            ImGui::Combo("Orbital 2", &orb2, orbitalNames, IM_ARRAYSIZE(orbitalNames));
            orbital2 = static_cast<OrbitalType>(orb2);

            ImGui::Checkbox("Hybrid Mode", &isHybrid);
            
            if (isHybrid) {
                ImGui::SliderFloat("Mix Factor", &mixFactor, 0.0f, 1.0f);
            }

            ImGui::Checkbox("Bonding State", &isBonding);

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

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 viewProj = projection * view;

        nucleus1.draw(viewProj, nucleusShader.ID);
        nucleus2.draw(viewProj, nucleusShader.ID);
        
        glDepthMask(GL_FALSE);
        cloud.draw(viewProj, cloudShader.ID);
        glDepthMask(GL_TRUE);

        // Orbital Information Display
        std::stringstream ss;
        if (!isHybrid) {
            ss << "Current State: Pure " << getOrbitalName(orbital1);
        } else {
            ss << "Hybrid State: " << std::fixed << std::setprecision(0) 
               << (1.0f - mixFactor) * 100 << "% [" << getOrbitalName(orbital1) << "] + " 
               << mixFactor * 100 << "% [" << getOrbitalName(orbital2) << "]";
        }
        ss << (isBonding ? " (Bonding)" : " (Anti-bonding)");

        textRenderer.RenderText(ss.str(), 20.0f, 565.0f, 0.55f, glm::vec3(1.0f, 1.0f, 1.0f));

        if (isHybrid) {
            std::string mixHint = "Mixing: " + getOrbitalName(orbital1) + " & " + getOrbitalName(orbital2);
            textRenderer.RenderText(mixHint, 20.0f, 540.0f, 0.45f, glm::vec3(0.7f, 0.7f, 0.7f));
        }

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
