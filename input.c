#include "input.h"
#include <stdio.h>

// TODO Debouncing

#define MAX_KEYS 512
static int keys[MAX_KEYS];
static int keysPressed[MAX_KEYS];

bool printKeyInfo = true;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (printKeyInfo)
        printf("Key: %d, Action: %d, Mods: %d\n", key, action, mods);

    if (key >= 0 && key < MAX_KEYS) {
        if (action == GLFW_PRESS) {
            if (!keys[key]) {
                keys[key] = 1;
                keysPressed[key] = 1;
                if (printKeyInfo)
                    printf("Key %d pressed\n", key);
            }
        } else if (action == GLFW_RELEASE) {
            keys[key] = 0;
            if (printKeyInfo)
                printf("Key %d released\n\n", key);
        }
    }
}

void initInput(GLFWwindow* window) {
    glfwSetKeyCallback(window, keyCallback);
    for (int i = 0; i < MAX_KEYS; i++) {
        keys[i] = 0;
        keysPressed[i] = 0;
    }
}

void updateInput() {
    for (int i = 0; i < MAX_KEYS; i++) {
        keysPressed[i] = 0;  // Reset only the pressed state
    }
}

int isKeyDown(int key) {
    if (key < 0 || key >= MAX_KEYS)
        return 0;
    return keys[key];
}

int isKeyPressed(int key) {
    if (key < 0 || key >= MAX_KEYS)
        return 0;
    return keysPressed[key];
}
