#ifndef MIRROR_H
#define MIRROR_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"

class Mirror
{
public:
	unsigned int framebuffer;
	unsigned int textureColorbuffer;
	unsigned int rbo;
	float camToMirrorLen;

	unsigned int scrHeight, scrWidth;

	//unsigned int quadVAO, quadVBO;
	glm::vec3 camPosition;
	glm::vec3 front;
	//Shader* screenShader;
	glm::vec3 mirrorPosition;
	glm::vec3 mirrorNormal;
	glm::mat4 m_model;

	Mirror(){}
	Mirror(glm::vec3 pos, glm::vec3 nom, unsigned int scr_width, unsigned int scr_height);
	~Mirror();

	void start_draw(Camera& camera);
	void end_draw(Camera& camera);

};

#endif // !MIRROR_H
