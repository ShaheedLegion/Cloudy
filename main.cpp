#include <Windows.h>
#include "Renderer.hpp"

namespace _impl {
// Fetched most of this code from: http://lodev.org/cgtutor/randomnoise.html

#define noiseDepth 16

#define RGB_S(b, g, r)                                                         \
  ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) |                         \
              (((DWORD)(BYTE)(b)) << 16)))

double noise[_WIDTH][_HEIGHT][noiseDepth]; // the noise array

void generateNoise() {
  for (int x = 0; x < _WIDTH; x++)
    for (int y = 0; y < _HEIGHT; y++)
      for (int z = 0; z < noiseDepth; z++) {
        noise[x][y][z] = (rand() % 32768) / 32768.0;
      }
}

inline double smoothNoise(double x, double y, double z) {
  // get fractional part of x and y
  double fractX = x - int(x);
  double fractY = y - int(y);
  double fractZ = z - int(z);

  // wrap around
  int x1 = (int(x) + _WIDTH) % _WIDTH;
  int y1 = (int(y) + _HEIGHT) % _HEIGHT;
  int z1 = (int(z) + noiseDepth) % noiseDepth;

  // neighbor values
  int x2 = (x1 + _WIDTH - 1) % _WIDTH;
  int y2 = (y1 + _HEIGHT - 1) % _HEIGHT;
  int z2 = (z1 + noiseDepth - 1) % noiseDepth;

  // smooth the noise with bilinear interpolation
  double value = 0.0;
  value += fractX * fractY * fractZ * noise[x1][y1][z1];
  value += fractX * (1 - fractY) * fractZ * noise[x1][y2][z1];
  value += (1 - fractX) * fractY * fractZ * noise[x2][y1][z1];
  value += (1 - fractX) * (1 - fractY) * fractZ * noise[x2][y2][z1];

  value += fractX * fractY * (1 - fractZ) * noise[x1][y1][z2];
  value += fractX * (1 - fractY) * (1 - fractZ) * noise[x1][y2][z2];
  value += (1 - fractX) * fractY * (1 - fractZ) * noise[x2][y1][z2];
  value += (1 - fractX) * (1 - fractY) * (1 - fractZ) * noise[x2][y2][z2];

  return value;
}

inline double turbulence(double x, double y, double z, double size) {
  double value = 0.0, initialSize = size;

  while (size >= 1) {
    value += smoothNoise(x / size, y / size, z / size) * size;
    size /= 2.0;
  }

  return (128.0 * value / initialSize);
}

typedef unsigned char uchar;

inline detail::Uint32 HSLtoRGB(uchar _h, uchar _s, uchar _l) {
  float r, g, b, h, s, l; // this function works with floats between 0 and 1
  float temp1, temp2, tempr, tempg, tempb;
  h = _h / 256.0;
  s = _s / 256.0;
  l = _l / 256.0;
  // If saturation is 0, the color is a shade of gray
  if (s == 0)
    r = g = b = l;
  // If saturation > 0, more complex calculations are needed
  else {
    // Set the temporary values
    if (l < 0.5)
      temp2 = l * (1 + s);
    else
      temp2 = (l + s) - (l * s);
    temp1 = 2 * l - temp2;
    tempr = h + 1.0 / 3.0;
    if (tempr > 1)
      tempr--;
    tempg = h;
    tempb = h - 1.0 / 3.0;
    if (tempb < 0)
      tempb++;

    // Red
    if (tempr < 1.0 / 6.0)
      r = temp1 + (temp2 - temp1) * 6.0 * tempr;
    else if (tempr < 0.5)
      r = temp2;
    else if (tempr < 2.0 / 3.0)
      r = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempr) * 6.0;
    else
      r = temp1;

    // Green
    if (tempg < 1.0 / 6.0)
      g = temp1 + (temp2 - temp1) * 6.0 * tempg;
    else if (tempg < 0.5)
      g = temp2;
    else if (tempg < 2.0 / 3.0)
      g = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempg) * 6.0;
    else
      g = temp1;

    // Blue
    if (tempb < 1.0 / 6.0)
      b = temp1 + (temp2 - temp1) * 6.0 * tempb;
    else if (tempb < 0.5)
      b = temp2;
    else if (tempb < 2.0 / 3.0)
      b = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempb) * 6.0;
    else
      b = temp1;
  }

  return RGB_S((r * 255.0), (g * 255.0), (b * 255.0));
}


class BitmapRenderer : public detail::IBitmapRenderer {
public:
	BitmapRenderer(){}
	~BitmapRenderer(){}

	void RenderToBitmap(HDC dc) override {
		SetTextColor(dc, 0);
		TextOut(dc, 0, 0, (LPCSTR)"Testing", 7);
	}
};

} // namespace _impl

// This is the guts of the renderer, without this it will do nothing.
DWORD WINAPI Update(LPVOID lpParameter) { // poll for some kind of event here to
                                          // signal that we want to exit.
  _impl::generateNoise();
  double t(0.0);
  Renderer *g_renderer = static_cast<Renderer *>(lpParameter);

  while (g_renderer->IsRunning()) {

    detail::Uint32 *buffer = g_renderer->screen.GetPixels();

    for (int x = 0; x < _WIDTH; x++) {
      for (int y = 0; y < _HEIGHT; y++) {
        unsigned int L(
            192 +
            static_cast<unsigned char>(_impl::turbulence(x, y, t, 32)) / 4);
        *buffer = _impl::HSLtoRGB(169, 255, L);
        buffer++;
      }
    }
    t += 3.0;

    g_renderer->screen.Flip();
    g_renderer->updateThread.Delay(1);
  }

  return 0;
}

int main(int argc, char *args[]) {
  const char *const myclass = "Cloudy";
  _impl::BitmapRenderer bmRenderer;
  Renderer renderer(myclass, &Update, &bmRenderer);

  return 0;
}
