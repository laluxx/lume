#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "theme.h"
#include "input.h"
#include <stdlib.h>
#include "renderer.h"
#include "common.h"
#include "font.h"
#include "window.h"

// TODO audio module

void old_framebuffer_size_callback(GLFWwindow* window, int width, int height);

int screenWidth, screenHeight;
/* Renderer renderer = {0}; // NOTE this has to be outside the main */

int main(void) {
    GLFWwindow* window;
    
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(640, 480, "Lume", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

    glfwSetFramebufferSizeCallback(window, old_framebuffer_size_callback);

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        glfwTerminate();
        return -1;
    }

    /* glEnable(GL_MULTISAMPLE); */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initRenderer(screenWidth, screenHeight);

    initInput(window);

    bool wireframe = false;

    GLuint puta = loadTexture("./puta.jpg");
    GLuint pengu = loadTexture("./pengu.png");

    
    initFreeType();
    Font *jetb = loadFont("radon.otf", 100);

    while (!glfwWindowShouldClose(window)) {
        
        /* glPointSize(4.0f); */

        if (isKeyDown(KEY_LEFT_ALT) || isKeyDown(KEY_RIGHT_ALT)) {
        }

        if (isKeyPressed(KEY_R)) {
            reloadShaders();
        }

        if (isKeyPressed(KEY_W)) {
            wireframe = !wireframe;
        }
        
        if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        
        updateInput();  // NOTE This should be called after all key handling
        reloadShaders(); // NOTE reloading shaders every frame its bad,

        glClearColor(BLACK.r, BLACK.g, BLACK.b, BLACK.a);
        glClear(GL_COLOR_BUFFER_BIT);



        useShader("circle");
        drawRectangle((Vec2f){screenWidth / 2.0 - 50.0f, screenHeight / 2.0 - 50.0f}, (Vec2f){100.0f, 100.0f}, RED);
        flush();



        useShader("simple");

        if (isGamepadButtonDown(GAMEPAD_BUTTON_A)) {
            drawRectangle((Vec2f){0, 21}, (Vec2f){screenWidth, 25}, RED);
        } else {
            drawRectangle((Vec2f){0, 21}, (Vec2f){screenWidth, 25}, CYAN);
        }

        flush();
        

        useShader("texture");
        drawTexture((Vec2f){400.0f, 400.0f,}, (Vec2f){300.0f, 300.0f}, pengu);
        flush();
        drawTexture((Vec2f){200.0f, 200.0f,}, (Vec2f){300.0f, 300.0f}, puta);
        flush();
        

        drawText(jetb, "Hello, World!", 800.0, 800.0, 1.0, 1.0);

        
        useShader("wave");
  
        drawTriangle(RED,
                     (Vec2f){100.0f, 100.0f},
                     (Vec2f){200.0f, 100.0f},
                     (Vec2f){150.0f, 200.0f});

        drawTriangleColors((Vec2f){100.0f, 50.0f}, RED, (Vec2f){1.0f, 0.0f},
                           (Vec2f){50.0f, 50.0f},  MAGENTA,     (Vec2f){0.0f, 0.0f},
                           (Vec2f){75.0f, 100.0f}, CYAN,       (Vec2f){0.5f, 1.0f});

        flush();



        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    freeRenderer();
    glfwTerminate();
    return 0;
}


bool oldPrintWindowSize = true;
void old_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    screenWidth = width;
    screenHeight = height;
    if(oldPrintWindowSize){
        printf("Width: %d Height: %d\n", screenWidth, screenHeight);
    }
    glViewport(0, 0, width, height);
    // Update the projection matrix
    updateProjectionMatrix(screenWidth, screenHeight);
}
