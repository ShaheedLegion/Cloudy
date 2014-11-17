#include "Renderer.hpp"

#include <Windows.h>
#include <cstdlib>
#include <vector>

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

void Renderer::SetDirection(int direction) { screen.SetDirection(direction); }

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

Renderer::Renderer(const char *const className, LPTHREAD_START_ROUTINE callback,
                   detail::IBitmapRenderer *renderer)
    : screen(_WIDTH, _HEIGHT, _BPP, renderer), updateThread(callback) {
  g_renderer = this;

  HDC windowDC;

  WNDCLASSEX wndclass = {sizeof(WNDCLASSEX), CS_DBLCLKS, WindowProcedure, 0, 0,
                         GetModuleHandle(0), LoadIcon(0, IDI_APPLICATION),
                         LoadCursor(0, IDC_ARROW), HBRUSH(COLOR_WINDOW + 1), 0,
                         className, LoadIcon(0, IDI_APPLICATION)};
  if (RegisterClassEx(&wndclass)) {
    // Get info on which monitor we want to use.
    std::vector<RECT> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc,
                        reinterpret_cast<DWORD>(&monitors));

    RECT displayRC = {0, 0, _WIDTH, _HEIGHT};
    std::vector<RECT>::reverse_iterator i = monitors.rbegin();
    if (i != monitors.rend())
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

Renderer::~Renderer() { updateThread.Join(); }

void Renderer::SetBuffer(unsigned char *buffer, HDC scrDC, HDC memDC) {
  screen.SetScreen(buffer, scrDC, memDC);
  updateThread.Start(static_cast<LPVOID>(this));
  SetRunning(true);
}

bool Renderer::IsRunning() { return bRunning; }

void Renderer::SetRunning(bool bRun) { bRunning = bRun; }
