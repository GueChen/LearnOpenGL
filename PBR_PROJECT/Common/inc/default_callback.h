#ifndef _DEFAULT_SCRIPT_H
#define _DEFAULT_SCRIPT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void DftScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void DftFrameSizeChangeCallback(GLFWwindow* window, int width, int height);

void DftCursorPosCallback(GLFWwindow* window, double xpos, double ypos);

extern bool   rotate_flag;
extern int    scr_width;
extern int    scr_height;

#endif // !_COMMON_SCRIPT_H
