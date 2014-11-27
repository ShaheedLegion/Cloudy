#pragma once
#ifndef RENDERER_HPP_INCLUDED
#define RENDERER_HPP_INCLUDED

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>

#define _WIDTH 256
#define _HEIGHT 256
#define _BPP 32

namespace detail {

class IBitmapRenderer {
public:
  virtual void RenderToBitmap(HDC screenDC) = 0;
  virtual void HandleOutput(VOID *output) = 0;
  virtual void HandleDirection(int direction) = 0;
};

class RendererThread {
public:
  RendererThread(LPTHREAD_START_ROUTINE callback)
      : m_running(false), hThread(INVALID_HANDLE_VALUE), m_callback(callback) {}

  ~RendererThread() {
    if (m_running)
      Join();
  }

  void Start(LPVOID lParam) {
    hThread = CreateThread(NULL, 0, m_callback, lParam, 0, NULL);
    m_running = true;
  }
  void Join() {
    if (m_running)
      m_running = false;

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    hThread = INVALID_HANDLE_VALUE;
  }

  void Delay(DWORD millis) {
    // Try to delay for |millis| time duration.
    // This is called from within the threading function (callback)
    // So it's safe to sleep in the calling thread.
    Sleep(millis);
  }

protected:
  bool m_running;
  HANDLE hThread;
  LPTHREAD_START_ROUTINE m_callback;
  // some protected stuff.
};

typedef unsigned int Uint32;

class RendererSurface {
public:
  RendererSurface(int w, int h, int bpp, IBitmapRenderer *renderer)
      : m_w(w), m_h(h), m_bpp(bpp), m_bitmapRenderer(renderer) {
    m_pixels = new unsigned char[m_w * m_h * (m_bpp / 8)];
    m_backBuffer = new unsigned char[m_w * m_h * (m_bpp / 8)];
  }

  ~RendererSurface() {
    delete[] m_pixels;
    delete[] m_backBuffer;
    m_pixels = NULL;
    m_backBuffer = NULL;
  }

  void SetScreen(unsigned char *buffer, HDC screenDC, HDC memDC) {
    m_screen = buffer;
    m_screenDC = screenDC;
    m_dc = memDC;
  }

  void SetDirection(int direction) {
    if (m_bitmapRenderer)
      m_bitmapRenderer->HandleDirection(direction);
  }

  void Flip() {
    // We need a mechanism to actually present the buffer to the drawing system.
    unsigned char *temp = m_pixels;
    m_pixels = m_backBuffer;
    m_backBuffer = temp;

    memcpy(m_screen, m_pixels, (m_w * m_h * (m_bpp / 8)));
    if (m_bitmapRenderer)
      m_bitmapRenderer->RenderToBitmap(m_dc);

    // BitBlt(m_screenDC, 0, 0, m_w, m_h, m_dc, 0, 0, SRCCOPY);
    StretchBlt(m_screenDC, 0, 0, m_w << 1, m_h << 1, m_dc, 0, 0, m_w, m_h,
               SRCCOPY);
  }

  Uint32 *GetPixels() { return reinterpret_cast<Uint32 *>(m_backBuffer); }

  int GetBPP() const { return m_bpp; }

  int GetWidth() const { return m_w; }

  int GetHeight() const { return m_h; }

protected:
  unsigned char *m_pixels;
  unsigned char *m_backBuffer;
  unsigned char *m_screen;
  int m_w;
  int m_h;
  int m_bpp;
  HDC m_screenDC;
  HDC m_dc;
  IBitmapRenderer *m_bitmapRenderer;
};

} // namespace detail

// Declaration and partial implementation.
class Renderer {
public:
  detail::RendererSurface screen;
  detail::RendererThread updateThread;

  bool bRunning;

  void SetBuffer(unsigned char *buffer, HDC scrDC, HDC memDC) {
    screen.SetScreen(buffer, scrDC, memDC);
    updateThread.Start(static_cast<LPVOID>(this));
    SetRunning(true);
  }
  void SetDirection(int direction) { screen.SetDirection(direction); }

public:
  Renderer(const char *const className, LPTHREAD_START_ROUTINE callback,
           detail::IBitmapRenderer *renderer);

  ~Renderer() { updateThread.Join(); }

  bool IsRunning() { return bRunning; }

  void SetRunning(bool bRun) { bRunning = bRun; }
};

// Here we declare the functions and variables used by the renderer instance
namespace forward {
Renderer *g_renderer;

void HandleKey(WPARAM wp) {
  switch (wp) {
  case VK_ESCAPE:
    if (g_renderer)
      g_renderer->SetRunning(false);
    PostQuitMessage(0);
    break;
  case VK_LEFT:
    if (g_renderer)
      g_renderer->SetDirection(0);
    break;
  case VK_UP:
    if (g_renderer)
      g_renderer->SetDirection(1);
    break;
  case VK_RIGHT:
    if (g_renderer)
      g_renderer->SetDirection(2);
    break;
  case VK_DOWN:
    if (g_renderer)
      g_renderer->SetDirection(3);
    break;
  default:
    break;
  }
}

long __stdcall WindowProcedure(HWND window, unsigned int msg, WPARAM wp,
                               LPARAM lp) {
  switch (msg) {
  case WM_DESTROY:
    PostQuitMessage(0);
    if (g_renderer)
      g_renderer->SetRunning(false);
    return 0L;
  case WM_KEYDOWN:
    HandleKey(wp);
    return 0L;
  default:
    return DefWindowProc(window, msg, wp, lp);
  }
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData) {
  if (!dwData || !lprcMonitor)
    return TRUE;

  std::vector<RECT> *monitors = reinterpret_cast<std::vector<RECT> *>(dwData);
  monitors->push_back(*lprcMonitor);

  return TRUE;
}
} // namespace forward

// Implementation of the renderer functions.
Renderer::Renderer(const char *const className, LPTHREAD_START_ROUTINE callback,
                   detail::IBitmapRenderer *renderer)
    : screen(_WIDTH, _HEIGHT, _BPP, renderer), updateThread(callback) {
  forward::g_renderer = this;

  HDC windowDC;

  WNDCLASSEX wndclass = {sizeof(WNDCLASSEX), CS_DBLCLKS,
                         forward::WindowProcedure, 0, 0, GetModuleHandle(0),
                         LoadIcon(0, IDI_APPLICATION), LoadCursor(0, IDC_ARROW),
                         HBRUSH(COLOR_WINDOW + 1), 0, className,
                         LoadIcon(0, IDI_APPLICATION)};
  if (RegisterClassEx(&wndclass)) {
    // Get info on which monitor we want to use.
    std::vector<RECT> monitors;
    EnumDisplayMonitors(NULL, NULL, forward::MonitorEnumProc,
                        reinterpret_cast<DWORD>(&monitors));

    RECT displayRC = {0, 0, _WIDTH, _HEIGHT};
    std::vector<RECT>::iterator i = monitors.begin();
    if (i != monitors.end())
      displayRC = *i;

    HWND window = CreateWindowEx(
        0, className, "Utility Renderer", WS_POPUPWINDOW, displayRC.left,
        displayRC.top, _WIDTH * 2, _HEIGHT * 2, 0, 0, GetModuleHandle(0), 0);
    if (window) {

      windowDC = GetWindowDC(window);
      HDC hImgDC = CreateCompatibleDC(windowDC);
      if (hImgDC == NULL)
        MessageBox(NULL, "Dc is NULL", "ERROR!", MB_OK);

      SetBkMode(hImgDC, TRANSPARENT);
      SetTextColor(hImgDC, RGB(255, 255, 255));
      SetStretchBltMode(hImgDC, COLORONCOLOR);

      BITMAPINFO bf;
      ZeroMemory(&bf, sizeof(BITMAPINFO));

      bf.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bf.bmiHeader.biWidth = _WIDTH;
      bf.bmiHeader.biHeight = _HEIGHT;
      bf.bmiHeader.biPlanes = 1;
      bf.bmiHeader.biBitCount = _BPP;
      bf.bmiHeader.biCompression = BI_RGB;
      bf.bmiHeader.biSizeImage = (_WIDTH * _HEIGHT * (_BPP / 8));
      bf.bmiHeader.biXPelsPerMeter = -1;
      bf.bmiHeader.biYPelsPerMeter = -1;

      unsigned char *bits;

      HBITMAP hImg = CreateDIBSection(hImgDC, &bf, DIB_RGB_COLORS,
                                      (void **)&bits, NULL, 0);
      if (hImg == NULL)
        MessageBox(NULL, "Image is NULL", "ERROR!", MB_OK);
      else if (hImg == INVALID_HANDLE_VALUE)
        MessageBox(NULL, "Image is invalid", "Error!", MB_OK);

      SelectObject(hImgDC, hImg);

      SetBuffer(bits, windowDC, hImgDC);

      ShowWindow(window, SW_SHOWDEFAULT);
      MSG msg;
      while (GetMessage(&msg, 0, 0, 0))
        DispatchMessage(&msg);
    }
  }
}

#endif // RENDERER_HPP_INCLUDED
