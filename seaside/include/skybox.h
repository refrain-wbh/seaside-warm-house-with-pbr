#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

#include "shader.h"

class Skybox
{
private:
    unsigned int VAO, VBO;

public:
    unsigned int textureSkybox;
    Shader skyboxShader;

public:
    Skybox(std::string vert_path, std::string frag_path);
    ~Skybox();

    void release();
    void render(glm::mat4 projection, glm::mat4 view);

};

#endif // !SKYBOX_H
