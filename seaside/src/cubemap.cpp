#include"CubeMap.h"
#include"stb_image.h"
glm::vec3 insideCameraPos{ 0.0f,0.0f,0.0f };
extern Camera camera;
Camera rightCamera(insideCameraPos, glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f);
Camera leftCamera(insideCameraPos, glm::vec3(0.0f, -1.0f, 0.0f), -180.0f, 0.0f);
Camera topCamera(insideCameraPos, glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 90.0f);
Camera bottomCamera(insideCameraPos, glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -90.0f);
Camera backCamera(insideCameraPos, glm::vec3(0.0f, -1.0f, 0.0f), 90.0f, 0.0f);
Camera frontCamera(insideCameraPos, glm::vec3(0.0f, -1.0f, 0.0f), -90.0f, 0.0f);
Camera temp_camera;
std::vector<Camera*> cameraList = { &rightCamera, &leftCamera, &topCamera, &bottomCamera,&backCamera ,&frontCamera, };
Cubemap::Cubemap()
{
    // create and bind FBO
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    unsigned int cubeBuffer;
    glGenTextures(1, &cubeBuffer);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeBuffer);
    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach it to currently bound FBO
    for (int i = 0; i < 6; ++i)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeBuffer, 0);
    }
    // generate renderbuffer (as depth buffer and stencil buffer)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 800);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // attach it to currently bound FBO
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Cubemap::draw()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // model, view, projection matrix setup
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
    temp_camera = camera;
    for (int i = 0; i < 6; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
        camera = *cameraList[i];

    }
}