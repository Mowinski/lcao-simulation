#ifndef NUCLEUS_HPP
#define NUCLEUS_HPP

#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>

class Nucleus {
public:
    Nucleus(float radius, int sectors, int stacks);
    ~Nucleus();

    void draw(const glm::mat4& viewProj, GLuint shaderProgram);
    void draw(const glm::mat4& viewProj, const glm::mat4& model, GLuint shaderProgram);
    void setPosition(const glm::vec3& pos) { position = pos; }
    glm::vec3 getPosition() const { return position; }

private:
    void setupMesh(float radius, int sectors, int stacks);

    GLuint vao, vbo, ebo;
    int indexCount;
    glm::vec3 position;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

#endif
