#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include <map>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

struct Character {
    GLuint TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

class TextRenderer {
public:
    std::map<char, Character> Characters;
    GLuint VAO, VBO;

    TextRenderer(GLuint width, GLuint height);
    void Load(std::string font, unsigned int fontSize);
    void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);

private:
    GLuint shaderProgram;
    glm::mat4 projection;
};

#endif
