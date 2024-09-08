#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "window.h"
#include "font.h"
#include "input.h"
#include <stdio.h>
#include <stdbool.h>

static GLFWwindow* g_window = NULL;

void char_callback(GLFWwindow* window, unsigned int codepoint) {
    if (currentTextInputCallback != NULL) {
        currentTextInputCallback(codepoint);  // Forward the codepoint to the registered handler
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (currentKeyInputCallback != NULL) {
        currentKeyInputCallback(key, action, mods);  // Forward the event to the registered handler
    }
}

GLFWwindow* initWindow(int width, int height, const char* title) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return NULL;
    }

    // Set window hints for OpenGL version and profile
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS

    // Create the window
    g_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!g_window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return NULL;
    }

    // Make the window's context current
    glfwMakeContextCurrent(g_window);
    glfwSetCharCallback(g_window, char_callback); // Set the character callback
    glfwSetKeyCallback(g_window, key_callback);   // Set key callback

    // Initialize GLEW
    /* glewExperimental = GL_TRUE;  // Enable GLEW experimental mode */
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(glewError));
        glfwDestroyWindow(g_window);
        glfwTerminate();
        return NULL;
    }

    // Check for OpenGL errors which might be caused by GLEW
    glGetError(); // Clear the error caused by GLEW initialization

    // Set V-Sync
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);


    // Set up a framebuffer size callback if needed
    /* glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback); */

    // Enable GL capabilities as needed
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initInput();
    initRenderer(width, height);
    initFreeType();
    return g_window;
}

int windowShouldClose() {
    if (!g_window) {
        fprintf(stderr, "Error: Window not initialized. Call initWindow() first.\n");
        return 1;
    }
    return glfwWindowShouldClose(g_window);
}

void beginDrawing() {
    // TODO Nothing here yet just an hook to the beginning of the frame
}

void clearBackground(Color color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void endDrawing() {
    updateInput();
    glfwSwapBuffers(g_window);
    glfwPollEvents();
}

void closeWindow() {
    if (g_window) {
        glfwDestroyWindow(g_window);
        g_window = NULL;
    }
    glfwTerminate();
}


int getScreenWidth() {
    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height);
    return width;
}

int getScreenHeight() {
    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height);
    return height;
}

double getTime() {
    return glfwGetTime();
}


#include <stdbool.h>

bool printWindowSize = true;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    if (printWindowSize) {
        printf("Width: %d Height: %d\n", width, height);
    }

    updateProjectionMatrix(width, height);
}
