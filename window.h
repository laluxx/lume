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

double getTime();
int getWindowAttribute(int attribute);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void char_callback(GLFWwindow* window, unsigned int codepoint);

extern bool swap_interval;
void setSwapInterval(bool enable);
void toggle_vsync();

extern bool alphaBlendingEnabled;
void enableAlphaBlending();
void disableAlphaBlending();
bool isAlphaBlendingEnabled();

#endif // WINDOW_H
