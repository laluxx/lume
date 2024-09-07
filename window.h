#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
#include "common.h"
#include "renderer.h"

GLFWwindow* initWindow(int width, int height, const char* title);
int windowShouldClose();
void beginDrawing();
void clearBackground(Color color);
void endDrawing();
void closeWindow();

int getScreenWidth();
int getScreenHeight();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);



#endif // WINDOW_H
