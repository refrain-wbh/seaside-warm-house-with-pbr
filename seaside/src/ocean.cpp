#include "ocean.h"

#include <cstdlib>

static float uniformRandom() {
	return (float)rand() / RAND_MAX;
}

//高斯随机
static glm::vec2 gaussianRandom2() {
	float x1, x2, w;
	do {
		x1 = 2.0f * uniformRandom() - 1.0f;
		x2 = 2.0f * uniformRandom() - 1.0f;
		w = x1 * x1 + x2 * x2;
	} while (w >= 1.0f);
	w = sqrt((-2.0f * log(w)) / w);
	return glm::vec2(x1 * w, x2 * w);
}


cOcean::cOcean(const int N, const float A, const glm::vec2 wind, const float length, const bool geometry, float scale_factor,
	const int nx, const int nz, const glm::vec3 offset, std::string vert_path, std::string frag_path) :
	g(9.81f), geometry(geometry), N(N), Nplus1(N + 1), A(A), wind(wind), length(length), m_scale_factor(scale_factor),
	m_nx(nx), m_nz(nz), m_offset(offset), oceanShader(vert_path.c_str(), frag_path.c_str()), textureSkybox(0),
	vertices(0), indices(0), h_tilde(0), h_tilde_slopex(0), h_tilde_slopez(0), h_tilde_dx(0), h_tilde_dz(0), fft(0)
{
	tStart = clock();
	compute_flag = false;
	m_speed_factor = 1.0f;

	h_tilde = new glm::vec2[N * N];
	h_tilde_slopex = new glm::vec2[N * N];
	h_tilde_slopez = new glm::vec2[N * N];
	h_tilde_dx = new glm::vec2[N * N];
	h_tilde_dz = new glm::vec2[N * N];
	fft = new cFFT(N);
	vertices = new vertex_ocean[Nplus1 * Nplus1];
	indices = new unsigned int[Nplus1 * Nplus1 * 10];

	modelMats = new glm::mat4[m_nz * m_nx];		//实例化数组

	int index;

	glm::vec2 htilde0, htilde0mk_conj;
	for (int m_prime = 0; m_prime < Nplus1; m_prime++) {
		for (int n_prime = 0; n_prime < Nplus1; n_prime++) {
			index = m_prime * Nplus1 + n_prime;

			htilde0 = hTilde_0(n_prime, m_prime);
			htilde0mk_conj = hTilde_0(-n_prime, m_prime);	//(-n,-m).conj();

			vertices[index].a = htilde0.x;
			vertices[index].b = htilde0.y;
			vertices[index]._a = htilde0mk_conj.x;
			vertices[index]._b = htilde0mk_conj.y;

			vertices[index].ox = vertices[index].x = (n_prime - N / 2.0f) * length / N;
			vertices[index].oy = vertices[index].y = 0.0f;
			vertices[index].oz = vertices[index].z = (m_prime - N / 2.0f) * length / N;

			vertices[index].nx = 0.0f;
			vertices[index].ny = 1.0f;
			vertices[index].nz = 0.0f;
		}
	}

	indices_count = 0;
	for (int m_prime = 0; m_prime < N; m_prime++) {
		for (int n_prime = 0; n_prime < N; n_prime++) {
			index = m_prime * Nplus1 + n_prime;

			if (geometry) {
				indices[indices_count++] = index;				// lines
				indices[indices_count++] = index + 1;
				indices[indices_count++] = index;
				indices[indices_count++] = index + Nplus1;
				indices[indices_count++] = index;
				indices[indices_count++] = index + Nplus1 + 1;
				if (n_prime == N - 1) {
					indices[indices_count++] = index + 1;
					indices[indices_count++] = index + Nplus1 + 1;
				}
				if (m_prime == N - 1) {
					indices[indices_count++] = index + Nplus1;
					indices[indices_count++] = index + Nplus1 + 1;
				}
			}
			else {
				indices[indices_count++] = index;				// two triangles
				indices[indices_count++] = index + Nplus1;
				indices[indices_count++] = index + Nplus1 + 1;
				indices[indices_count++] = index;
				indices[indices_count++] = index + Nplus1 + 1;
				indices[indices_count++] = index + 1;
			}
		}
	}

	glGenVertexArrays(1, &VAO);		//VAO

	glGenBuffers(1, &VBO);			//VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_ocean) * (Nplus1) * (Nplus1), vertices, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &EBO);			//EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(unsigned int), indices, GL_STATIC_DRAW);
}

cOcean::~cOcean()
{
	if (h_tilde)		delete[] h_tilde;
	if (h_tilde_slopex)	delete[] h_tilde_slopex;
	if (h_tilde_slopez)	delete[] h_tilde_slopez;
	if (h_tilde_dx)		delete[] h_tilde_dx;
	if (h_tilde_dz)		delete[] h_tilde_dz;
	if (fft)			delete fft;
	if (vertices)		delete[] vertices;
	if (indices)		delete[] indices;
	if (modelMats)		delete[] modelMats;
}

void cOcean::release()
{
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteProgram(oceanShader.ID);
}

float cOcean::dispersion(int n_prime, int m_prime)
{
	float w_0 = float(2.0f * M_PI / 200.0f);
	float kx = float(M_PI * (2 * (long long)n_prime - N) / length);
	float kz = float(M_PI * (2 * (long long)m_prime - N) / length);
	return floor(sqrt(g * sqrt(kx * kx + kz * kz)) / w_0) * w_0;
}

float cOcean::phillips(int n_prime, int m_prime)
{
	glm::vec2 k(float(M_PI * (2 * (long long)n_prime - N) / length),
		float(M_PI * (2 * (long long)m_prime - N) / length));
	float k_length = glm::length(k);
	if (k_length < 0.000001) return 0.0;

	float k_length2 = k_length * k_length;
	float k_length4 = k_length2 * k_length2;

	
	float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(wind));	//k.unit() * wind.unit();
	float k_dot_w2 = k_dot_w * k_dot_w * k_dot_w * k_dot_w * k_dot_w * k_dot_w;

	float w_length = glm::length(wind);
	float L = w_length * w_length / g;
	float L2 = L * L;

	float damping = 0.001f;
	float l2 = L2 * damping * damping;

	return A * exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2 * exp(-k_length2 * l2);
}

glm::vec2 cOcean::hTilde_0(int n_prime, int m_prime)
{
	glm::vec2 r = gaussianRandom2();
	return r * sqrt(phillips(n_prime, m_prime) / 2.0f);
}

glm::vec2 cOcean::hTilde(float t, int n_prime, int m_prime)
{
	int index = m_prime * Nplus1 + n_prime;

	glm::vec2 htilde0(vertices[index].a, vertices[index].b);
	glm::vec2 htilde0mkconj(vertices[index]._a, vertices[index]._b);

	float omegat = dispersion(n_prime, m_prime) * t;

	float cos_ = cos(omegat);
	float sin_ = sin(omegat);

	glm::vec2 c0(cos_, sin_);
	glm::vec2 c1(cos_, -sin_);

	return z_mult(htilde0, c0) + z_mult(htilde0mkconj, c1);
}

complex_vector_normal cOcean::h_D_and_n(glm::vec2 x, float t)
{
	glm::vec2 h(0.0f, 0.0f);
	glm::vec2 D(0.0f, 0.0f);
	glm::vec3 n(0.0f, 0.0f, 0.0f);

	glm::vec2 c, htilde_c;
	glm::vec2 k;
	float kx, kz, k_length, k_dot_x;

	for (int m_prime = 0; m_prime < N; m_prime++) {
		kz = float(M_PI * (2 * (long long)m_prime - N) / length);
		for (int n_prime = 0; n_prime < N; n_prime++) {
			kx = float(M_PI * (2 * (long long)n_prime - N) / length);
			k = glm::vec2(kx, kz);

			k_length = glm::length(k);
			k_dot_x = glm::dot(k, x);	//k * x;

			c = glm::vec2(cos(k_dot_x), sin(k_dot_x));
			htilde_c = z_mult(hTilde(t, n_prime, m_prime), c);

			h = h + htilde_c;

			n = n + glm::vec3(-kx * htilde_c.y, 0.0f, -kz * htilde_c.y);

			if (k_length < 0.000001) continue;
			D = D + glm::vec2(kx / k_length * htilde_c.y, kz / k_length * htilde_c.y);
		}
	}

	n = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) - n);	//(vec3 - n).unit();

	complex_vector_normal cvn;
	cvn.h = h;
	cvn.D = D;
	cvn.n = n;
	return cvn;
}

void cOcean::evaluateWavesFFT(float t)
{
	float kx, kz, len, lambda = -1.0f;
	int index, index1;

	for (int m_prime = 0; m_prime < N; m_prime++) {
		kz = float(M_PI * (2 * (long long)m_prime - N) / length);
		for (int n_prime = 0; n_prime < N; n_prime++) {
			kx = float(M_PI * (2 * (long long)n_prime - N) / length);
			len = sqrt(kx * kx + kz * kz);
			index = m_prime * N + n_prime;

			h_tilde[index] = hTilde(t, n_prime, m_prime);
			h_tilde_slopex[index] = z_mult(h_tilde[index], glm::vec2(0, kx));
			h_tilde_slopez[index] = z_mult(h_tilde[index], glm::vec2(0, kz));
			if (len < 0.000001f) {
				h_tilde_dx[index] = glm::vec2(0.0f, 0.0f);
				h_tilde_dz[index] = glm::vec2(0.0f, 0.0f);
			}
			else {
				h_tilde_dx[index] = z_mult(h_tilde[index], glm::vec2(0, -kx/len));
				h_tilde_dz[index] = z_mult(h_tilde[index], glm::vec2(0, -kz/len));
			}
		}
	}

	for (int m_prime = 0; m_prime < N; m_prime++) {
		fft->fft(h_tilde, h_tilde, 1, m_prime * N);
		fft->fft(h_tilde_slopex, h_tilde_slopex, 1, m_prime * N);
		fft->fft(h_tilde_slopez, h_tilde_slopez, 1, m_prime * N);
		fft->fft(h_tilde_dx, h_tilde_dx, 1, m_prime * N);
		fft->fft(h_tilde_dz, h_tilde_dz, 1, m_prime * N);
	}
	for (int n_prime = 0; n_prime < N; n_prime++) {
		fft->fft(h_tilde, h_tilde, N, n_prime);
		fft->fft(h_tilde_slopex, h_tilde_slopex, N, n_prime);
		fft->fft(h_tilde_slopez, h_tilde_slopez, N, n_prime);
		fft->fft(h_tilde_dx, h_tilde_dx, N, n_prime);
		fft->fft(h_tilde_dz, h_tilde_dz, N, n_prime);
	}

	int sign;
	float signs[] = { 1.0f, -1.0f };
	glm::vec3 n;
	for (int m_prime = 0; m_prime < N; m_prime++) {
		for (int n_prime = 0; n_prime < N; n_prime++) {
			index = m_prime * N + n_prime;		// index into h_tilde..
			index1 = m_prime * Nplus1 + n_prime;	// index into vertices

			sign = int(signs[(n_prime + m_prime) & 1]);

			h_tilde[index] = h_tilde[index] * float(sign);

			// height
			vertices[index1].y = h_tilde[index].x;

			// displacement
			h_tilde_dx[index] = h_tilde_dx[index] * float(sign);
			h_tilde_dz[index] = h_tilde_dz[index] * float(sign);
			vertices[index1].x = vertices[index1].ox + h_tilde_dx[index].x * lambda;
			vertices[index1].z = vertices[index1].oz + h_tilde_dz[index].x * lambda;

			// normal
			h_tilde_slopex[index] = h_tilde_slopex[index] * float(sign);
			h_tilde_slopez[index] = h_tilde_slopez[index] * float(sign);
			n = glm::normalize(glm::vec3(0.0f - h_tilde_slopex[index].x, 1.0f, 0.0f - h_tilde_slopez[index].x));	//.unit();
			vertices[index1].nx = n.x;
			vertices[index1].ny = n.y;
			vertices[index1].nz = n.z;

			// for tiling
			if (n_prime == 0 && m_prime == 0) {
				vertices[index1 + N + Nplus1 * N].y = h_tilde[index].x;

				vertices[index1 + N + Nplus1 * N].x = vertices[index1 + N + Nplus1 * N].ox + h_tilde_dx[index].x * lambda;
				vertices[index1 + N + Nplus1 * N].z = vertices[index1 + N + Nplus1 * N].oz + h_tilde_dz[index].x * lambda;

				vertices[index1 + N + Nplus1 * N].nx = n.x;
				vertices[index1 + N + Nplus1 * N].ny = n.y;
				vertices[index1 + N + Nplus1 * N].nz = n.z;
			}
			if (n_prime == 0) {
				vertices[index1 + N].y = h_tilde[index].x;

				vertices[index1 + N].x = vertices[index1 + N].ox + h_tilde_dx[index].x * lambda;
				vertices[index1 + N].z = vertices[index1 + N].oz + h_tilde_dz[index].x * lambda;

				vertices[index1 + N].nx = n.x;
				vertices[index1 + N].ny = n.y;
				vertices[index1 + N].nz = n.z;
			}
			if (m_prime == 0) {
				vertices[index1 + Nplus1 * N].y = h_tilde[index].x;

				vertices[index1 + Nplus1 * N].x = vertices[index1 + Nplus1 * N].ox + h_tilde_dx[index].x * lambda;
				vertices[index1 + Nplus1 * N].z = vertices[index1 + Nplus1 * N].oz + h_tilde_dz[index].x * lambda;

				vertices[index1 + Nplus1 * N].nx = n.x;
				vertices[index1 + Nplus1 * N].ny = n.y;
				vertices[index1 + Nplus1 * N].nz = n.z;
			}
		}
	}
}

void cOcean::render(float t, glm::mat4 projection, glm::mat4 view)
{
	if (compute_flag == true)
		evaluateWavesFFT(t * m_speed_factor);

	oceanShader.use();
	oceanShader.setInt("skybox", 0);

	oceanShader.setVec3("light_position", sunlight_position);
	oceanShader.setMat4("projection", projection);
	oceanShader.setMat4("view", view);

	//绑定
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex_ocean) * Nplus1 * Nplus1, vertices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	//顶点属性
	//位置
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), 0);
	//法线
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), (void*)(3 * sizeof(float)));
	//纹理坐标
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), (void*)(6 * sizeof(float)));

	//天空盒贴图
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureSkybox);

	float scale_factor = m_scale_factor;	//缩放
	glm::mat4 model0 = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));
#if 0
	//######## 循环绘制(需修改vert着色器，uniform model)
	for (int j = 0; j < m_nz; j++) {
		for (int i = 0; i < m_nx; i++) {
			glm::mat4 model = glm::translate(model0, glm::vec3(length * i + m_offset.x, m_offset.y, length * -j + m_offset.z));
			oceanShader.setMat4("model", model);
			glDrawElements(geometry ? GL_LINES : GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);
		}
	}
#else
	//######## 实例化
	//实例化数组赋值
	int index = 0;
	for (int j = 0; j < m_nz; j++) {
		for (int i = 0; i < m_nx; i++) {
			glm::mat4 model = glm::translate(model0, glm::vec3(length * i + m_offset.x, m_offset.y, length * -j + m_offset.z));
			modelMats[index++] = model;
		}
	}
	//绑定
	unsigned int instanceVBO;
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * m_nz * m_nx, &modelMats[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//属性
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	//绘制
	glBindVertexArray(VAO);
	glDrawElementsInstanced(geometry ? GL_LINES : GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0, m_nz * m_nx);
#endif

	return;
}

void cOcean::resetTimer()
{
	tStart = clock();
}

float cOcean::deltaTime()
{
	time_t current = clock();		//毫秒
	return float((current - tStart) / 1000.0);		//秒
}
