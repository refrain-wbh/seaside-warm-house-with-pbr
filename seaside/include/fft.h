#ifndef FFT_H
#define FFT_H

#include <glm/glm.hpp>
#include <math.h>

#ifndef  M_PI
#define M_PI 3.1415926	//3.14159265358979323846
#endif // ! M_PI

glm::vec2 z_mult(glm::vec2 z1, glm::vec2 z2);

class cFFT {
  private:
	unsigned int N, which;
	unsigned int log_2_N;
	float pi2;
	unsigned int *reversed;
	glm::vec2 **T;
	glm::vec2 *c[2];
  protected:
  public:
	cFFT(unsigned int N);
	~cFFT();
	unsigned int reverse(unsigned int i);
	glm::vec2 t(unsigned int x, unsigned int N);
	void fft(glm::vec2* input, glm::vec2* output, int stride, int offset);
};

#endif