#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "camera.h"

extern float deltaTime, lastFrame;

extern bool firstMouse;
extern float lastX, lastY;
extern float pitch, yaw;
extern float fov;
extern int g_speed;

extern Camera camera;

//处理按键输入
void processInput(GLFWwindow* window);

//GLFW初始化
void InitWindow(GLFWwindow*& window, int width, int height, const char* title);

//加载纹理
unsigned int loadTexture(char const* path);

//加载立方体贴图
unsigned int loadCubemap(std::vector<std::string> faces);
