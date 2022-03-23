#ifndef OCEAN_H
#define OCEAN_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <time.h>

#include "shader.h"
#include "fft.h"

struct vertex_ocean {
	GLfloat   x, y, z;		// 位置
	GLfloat  nx, ny, nz;	// 法线
	GLfloat   a, b, c;		// htilde0 纹理坐标(a,b)
	GLfloat  _a, _b, _c;	// htilde0mk conjugate
	GLfloat  ox, oy, oz;	// original position
};

struct complex_vector_normal {	// structure used with discrete fourier transform
	glm::vec2 h;		// wave height
	glm::vec2 D;		// displacement
	glm::vec3 n;		// normal
};

class cOcean {
private:
	bool geometry;			// 1:线框/0:表面

	float g;				// 重力加速度
	int N, Nplus1;			// nFFT点数（N为2的幂）
	float A;				// 振幅
	glm::vec2 wind;			// 风速、风向

	int m_nx, m_nz;			// 海面块数(nx,nz)*单块面积=海面大小
	glm::vec3 m_offset;		// 海面位置偏移

	float length;			// length parameter
	glm::vec2* h_tilde,		// for fast fourier transform
		* h_tilde_slopex, * h_tilde_slopez,
		* h_tilde_dx, * h_tilde_dz;
	cFFT* fft;				// fast fourier transform

	time_t tStart;
	
	glm::mat4* modelMats;	//实例化数组

	vertex_ocean* vertices;				// vertices for vertex buffer object
	unsigned int* indices;				// indicies for vertex buffer object
	unsigned int indices_count;			// number of indices to render

	GLuint VAO, VBO, EBO;

public:
	Shader oceanShader;
	unsigned int textureSkybox;

	float m_scale_factor;
	float m_speed_factor;
	glm::vec3 sunlight_position;

	bool compute_flag;

protected:
public:
	cOcean(const int N, const float A, const glm::vec2 wind, const float length, bool geometry, float scale_factor,
		const int nx, const int nz, const glm::vec3 offset, std::string vert_path, std::string frag_path);
	~cOcean();
	void release();

	float dispersion(int n_prime, int m_prime);		// deep water
	float phillips(int n_prime, int m_prime);		// phillips spectrum
	glm::vec2 hTilde_0(int n_prime, int m_prime);
	glm::vec2 hTilde(float t, int n_prime, int m_prime);
	complex_vector_normal h_D_and_n(glm::vec2 x, float t);

	void evaluateWavesFFT(float t);
	void render(float t, glm::mat4 projection, glm::mat4 view);

	void resetTimer();
	float deltaTime();
};

#endif	// !OCEAN_H