#include "input.h"
#include <stdio.h>

// TODO Debouncing

#define MAX_KEYS 512
static int keys[MAX_KEYS];
static int keysPressed[MAX_KEYS];

#define MAX_GAMEPAD_BUTTONS 15
#define MAX_GAMEPAD_AXES 6
static int gamepadButtons[MAX_GAMEPAD_BUTTONS];
static int gamepadButtonsPressed[MAX_GAMEPAD_BUTTONS];
static float gamepadAxes[MAX_GAMEPAD_AXES];

bool printKeyInfo = true;

void initInput() {
    for (int i = 0; i < MAX_KEYS; i++) {
        keys[i] = 0;
        keysPressed[i] = 0;
    }
}

static void gamepadUpdate() {
    GLFWgamepadstate state;
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
        for (int i = 0; i < MAX_GAMEPAD_BUTTONS; i++) {
            int newState = state.buttons[i];
            if (newState == GLFW_PRESS && gamepadButtons[i] == 0) {
                gamepadButtons[i] = 1;
                gamepadButtonsPressed[i] = 1;
                if (printKeyInfo)
                    printf("Gamepad Button %d pressed\n", i);
            } else if (newState == GLFW_RELEASE && gamepadButtons[i] == 1) {
                gamepadButtons[i] = 0;
                if (printKeyInfo)
                    printf("Gamepad Button %d released\n", i);
            }
        }
        for (int i = 0; i < MAX_GAMEPAD_AXES; i++) {
            gamepadAxes[i] = state.axes[i];
        }
    }
}

void updateInput() {
    for (int i = 0; i < MAX_KEYS; i++) {
        keysPressed[i] = 0;  // Reset only the pressed state for keys
    }
    for (int i = 0; i < MAX_GAMEPAD_BUTTONS; i++) {
        gamepadButtonsPressed[i] = 0;  // Reset only the pressed state for gamepad buttons
    }
    gamepadUpdate();  // Update gamepad state
}


int isGamepadButtonPressed(int button) {
    if (button < 0 || button >= MAX_GAMEPAD_BUTTONS)
        return 0;
    return gamepadButtonsPressed[button];
}

int isGamepadButtonDown(int button) {
    if (button < 0 || button >= MAX_GAMEPAD_BUTTONS)
        return 0;
    return gamepadButtons[button];
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

// Global variable to store the text input callback
TextInputCallback currentTextInputCallback = NULL;

void registerTextInputCallback(TextInputCallback callback) {
    currentTextInputCallback = callback;
}


// Global variable to store the key input callback
KeyInputCallback currentKeyInputCallback = NULL;

void registerKeyInputCallback(KeyInputCallback callback) {
    currentKeyInputCallback = callback;
}



