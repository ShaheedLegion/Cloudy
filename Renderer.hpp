#pragma once
#ifndef RENDERER_HPP_INCLUDED
#define RENDERER_HPP_INCLUDED

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define _WIDTH 512
#define _HEIGHT 512
#define _BPP 32

namespace detail {

class RendererThread {
public:
	RendererThread(LPTHREAD_START_ROUTINE callback) : m_running(false),
		hThread(INVALID_HANDLE_VALUE), m_callback(callback) {}

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
	//some protected stuff.
};


typedef unsigned int Uint32;

class RendererSurface {
public:
	RendererSurface(int w, int h, int bpp) : m_w(w), m_h(h), m_bpp(bpp) {
		m_pixels = new unsigned char[m_w * m_h * (m_bpp / 8)];
		m_backBuffer = new unsigned char[m_w * m_h * (m_bpp / 8)];
	}

	~RendererSurface() {
		delete [] m_pixels;
		delete [] m_backBuffer;
		m_pixels = NULL;
		m_backBuffer = NULL;
	}


	void SetScreen(unsigned char * buffer, HDC screenDC, HDC memDC) {
		m_screen = buffer;
		m_screenDC = screenDC;
		m_dc = memDC;
	}

	void Flip() {
		//We need a mechanism to actually present the buffer to the drawing system.
		unsigned char * temp = m_pixels;
		m_pixels = m_backBuffer;
		m_backBuffer = temp;

		memcpy(m_screen, m_pixels, (m_w * m_h * (m_bpp / 8)));
		BitBlt(m_screenDC, 0, 0, m_w, m_h, m_dc, 0, 0, SRCCOPY);
	}

	Uint32* GetPixels() {
		return reinterpret_cast<Uint32*>(m_backBuffer);
	}

	int GetBPP() const {
		return m_bpp;
	}

	int GetWidth() const {
		return m_w;
	}

	int GetHeight() const {
		return m_h;
	}

protected:
	unsigned char* m_pixels;
	unsigned char* m_backBuffer;
	unsigned char* m_screen;
	int m_w;
	int m_h;
	int m_bpp;
	HDC m_screenDC;
	HDC m_dc;
};

}  // namespace detail

class Renderer
{
public:
	detail::RendererSurface screen;
	detail::RendererThread updateThread;

    bool bRunning;

	void SetBuffer(unsigned char * buffer, HDC scrDC, HDC memDC);

public:
    Renderer(const char* const className, LPTHREAD_START_ROUTINE callback);
    ~Renderer();
    bool IsRunning();
    void SetRunning(bool);
};

#endif // RENDERER_HPP_INCLUDED
