#pragma once
#ifndef CUBEMAP_H
#define CUBEMAP_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "camera.h"
#include<string>
#include<vector>
class Cubemap
{
	// create and bind FBO
	
	Cubemap();
	void draw();
};
#endif // CUBEMAP_H

