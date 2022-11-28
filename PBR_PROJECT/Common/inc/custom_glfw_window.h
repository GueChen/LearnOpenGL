#ifndef __GLFW_CUSTOM_WINDOW_H
#define __GLFW_CUSTOM_WINDOW_H

#include "default_callback.h"

#include <cinttypes>
#include <cassert>

class GLFWCustomWindow {
public:
	GLFWCustomWindow(const char* title, uint32_t width, uint32_t height);
	~GLFWCustomWindow() {					
		glfwTerminate();					
	}		
	void Run();

	inline void RegisterCursorPosFunc(GLFWcursorposfun func)       { glfwSetCursorPosCallback(m_window_ptr, func); }
	inline void RegisterFBSizeFunc   (GLFWframebuffersizefun func) { glfwSetFramebufferSizeCallback(m_window_ptr, func); }
	inline void RegisterScrollFunc   (GLFWscrollfun func)          { glfwSetScrollCallback(m_window_ptr, func); }	

public:
	GLFWwindow* m_window_ptr = nullptr;
	void(*m_loop_func)(void) = nullptr;	
};

GLFWCustomWindow::GLFWCustomWindow(const char* title, uint32_t width, uint32_t height) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

	m_window_ptr = glfwCreateWindow(width, height, title, NULL, NULL);
	
	if (m_window_ptr == NULL) {
		glfwTerminate();
	}
	
	glfwMakeContextCurrent(m_window_ptr);
	
	RegisterCursorPosFunc(DftCursorPosCallback);
	RegisterFBSizeFunc   (DftFrameSizeChangeCallback);
	RegisterScrollFunc   (DftScrollCallback);

	assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));

	// no cursor display
	glfwSetInputMode(m_window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void GLFWCustomWindow::Run() {
	assert(m_window_ptr && "The window ptr is always null, error");
	assert(m_loop_func  && "Loop func not set");
	while (!glfwWindowShouldClose(m_window_ptr)) {
		m_loop_func();
		glfwSwapBuffers(m_window_ptr);
		glfwPollEvents();
	}
}

#endif // !__GLFW_CUSTOM_WINDOW_H


