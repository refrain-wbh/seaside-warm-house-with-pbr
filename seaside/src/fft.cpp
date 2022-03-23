#include "fft.h"

//¸´Êý³Ë·¨
glm::vec2 z_mult(glm::vec2 z1, glm::vec2 z2)
{
	return glm::vec2(z1.x*z2.x-z1.y*z2.y, z1.x*z2.y+z1.y*z2.x);
}

cFFT::cFFT(unsigned int N) : N(N), reversed(0), T(0), pi2(float(2 * M_PI)) {
	c[0] = c[1] = 0;

	log_2_N = (unsigned int)(log(N)/log(2));

	reversed = new unsigned int[N];		// prep bit reversals
	for (int i = 0; i < int(N); i++) reversed[i] = reverse(i);

	int pow2 = 1;
	T = new glm::vec2*[log_2_N];		// prep T
	for (int i = 0; i < int(log_2_N); i++) {
		T[i] = new glm::vec2[pow2];
		for (int j = 0; j < pow2; j++) T[i][j] = t(j, pow2 * 2);
		pow2 *= 2;
	}

	c[0] = new glm::vec2[N];
	c[1] = new glm::vec2[N];
	which = 0;
}

cFFT::~cFFT() {
	if (c[0]) delete [] c[0];
	if (c[1]) delete [] c[1];
	if (T) {
		for (int i = 0; i < int(log_2_N); i++) if (T[i]) delete [] T[i];
		delete [] T;
	}
	if (reversed) delete [] reversed;
}

unsigned int cFFT::reverse(unsigned int i) {
	unsigned int res = 0;
	for (int j = 0; j < int(log_2_N); j++) {
		res = (res << 1) + (i & 1);
		i >>= 1;
	}
	return res;
}

glm::vec2 cFFT::t(unsigned int x, unsigned int N) {
	return glm::vec2(float(cos(pi2 * x / N)), float(sin(pi2 * x / N)));
}

void cFFT::fft(glm::vec2* input, glm::vec2* output, int stride, int offset) {
	for (int i = 0; i < int(N); i++) c[which][i] = input[reversed[i] * stride + offset];

	int loops       = N>>1;
	int size        = 1<<1;
	int size_over_2 = 1;
	int w_          = 0;
	for (int i = 1; i <= int(log_2_N); i++) {
		which ^= 1;
		for (int j = 0; j < loops; j++) {
			for (int k = 0; k < size_over_2; k++) {
				c[which][size * j + k] =  c[which^1][size * j + k] +
					z_mult(c[which^1][size * j + size_over_2 + k], T[w_][k]);
			}

			for (int k = size_over_2; k < size; k++) {
				c[which][size * j + k] =  c[which^1][size * j - size_over_2 + k] -
					z_mult(c[which^1][size * j + k], T[w_][k - size_over_2]);
			}
		}
		loops       >>= 1;
		size        <<= 1;
		size_over_2 <<= 1;
		w_++;
	}

	for (int i = 0; i < int(N); i++) output[i * stride + offset] = c[which][i];
}