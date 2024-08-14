#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "theme.h"
#include "input.h"
#include <stdlib.h>
#include "renderer.h"
#include "common.h"

// TODO audio module

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int screenWidth, screenHeight;
Renderer renderer = {0}; // NOTE this has to be outside the main

int main(void) {
    GLFWwindow* window;


    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(640, 480, "Lume", NULL, NULL);
    if (!window)
        {
            glfwTerminate();
            return -1;
        }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        glfwTerminate();
        return -1;
    }
    
    initRenderer(&renderer, screenWidth, screenHeight);

    initInput(window);
    initializeThemes();
    
    glfwWindowHint(GLFW_SAMPLES, 4); // Set 4x MSAA
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);


    while (!glfwWindowShouldClose(window)) {

        /* glPointSize(4.0f); */
        /* glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); */


        if (isKeyDown(KEY_LEFT_ALT) || isKeyDown(KEY_RIGHT_ALT)) {
            if (isKeyPressed(KEY_EQUAL)) nextTheme();
            if (isKeyPressed(KEY_MINUS)) previousTheme();
        }

        if (isKeyPressed(KEY_R)) {
            reloadShaders(&renderer);
        }

        updateInput();  // NOTE This should be called after all key handling

        reloadShaders(&renderer); // TODO reloading shaders every frame its bad,


        glClearColor(CURRENT_THEME.bg.r, CURRENT_THEME.bg.g, CURRENT_THEME.bg.b, CURRENT_THEME.bg.a);
        glClear(GL_COLOR_BUFFER_BIT);

        useShader(&renderer, "circle");
        drawRectangle(&renderer, (Vec2f){screenWidth / 2.0 - 50.0f, screenHeight / 2.0 - 50.0f}, (Vec2f){100.0f, 100.0f}, CURRENT_THEME.cursor);
        flush(&renderer);




        useShader(&renderer, "simple");

        if (isGamepadButtonDown(GAMEPAD_BUTTON_A)) {
            drawRectangle(&renderer, (Vec2f){0, 21}, (Vec2f){screenWidth, 25}, CURRENT_THEME.modeline_highlight);
        } else {
            drawRectangle(&renderer, (Vec2f){0, 21}, (Vec2f){screenWidth, 25}, CURRENT_THEME.cursor);
        }
        
        flush(&renderer);

        
        useShader(&renderer, "wave");
  
        drawTriangle(&renderer, CURRENT_THEME.cursor,
                     (Vec2f){100.0f, 100.0f},
                     (Vec2f){200.0f, 100.0f},
                     (Vec2f){150.0f, 200.0f});

        drawTriangleColors(&renderer,
                           (Vec2f){100.0f, 50.0f}, CURRENT_THEME.minibuffer, (Vec2f){1.0f, 0.0f},
                           (Vec2f){50.0f, 50.0f},  CURRENT_THEME.cursor,     (Vec2f){0.0f, 0.0f},
                           (Vec2f){75.0f, 100.0f}, CURRENT_THEME.text,       (Vec2f){0.5f, 1.0f});

        flush(&renderer);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    freeRenderer(&renderer);
    glfwTerminate();
    return 0;
}


bool printWindowSize = true;
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    screenWidth = width;
    screenHeight = height;
    if(printWindowSize){
        printf("Width: %d Height: %d\n", screenWidth, screenHeight);
    }
    glViewport(0, 0, width, height);
    // Update the projection matrix
    updateProjectionMatrix(&renderer, screenWidth, screenHeight);
}
