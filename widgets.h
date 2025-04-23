#ifndef WIDGETS_H
#define WIDGETS_H

#include "renderer.h"
#include "common.h"
#include "font.h"
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Widget Widget;
typedef struct Div Div;
typedef void (*WidgetClickCallback)(Widget* widget);
typedef void (*WidgetTextCallback)(Widget *widget, const char *text);


extern Font *widgetsFont;

// NOTE All UI elements inherit from this
struct Widget {
    // Core properties
    char* id;
    bool visible;
    bool active;
    float alpha;  // Transparency (0.0-1.0)
    
    // Hierarchy
    Widget* parent;
    Widget* firstChild;
    Widget* lastChild;
    Widget* nextSibling;
    Widget* prevSibling;
    
    // Layout
    Vec2f position;
    Vec2f size;
    Vec2f minSize;
    Vec2f maxSize;
    
    // Appearance
    Color bgColor;
    Color fgColor;
    Color borderColor;
    float borderWidth;
    float cornerRadius;
    
    // State
    bool isHovered;
    bool isDown;

    // TODO
    /* bool movable; */
    /* bool movable_outside_div; */
    /* bool ontop; */
    /* bool pinned; */
    
    // Callbacks
    WidgetClickCallback onClick;
    WidgetTextCallback onTextInput;
    
    // Virtual function pointers
    void (*draw)(Widget* widget);
    void (*update)(Widget* widget, float dt);
    void (*free)(Widget* widget);
    void (*layout)(Widget* widget);  // For responsive layout
};

struct Div {
    Widget base;  // Everything is a widget
    Vec2f scroll;
};

// Button structure
typedef struct {
    Widget base;
    char* label;
    Color normalColor;
    Color hoverColor;
    Color pressedColor;
    Color textColor;
    float fontSize;
} Button;

// Core functions
Widget* createWidget(const char* id, Vec2f position, Vec2f size);
Div* createDiv(const char* id, Vec2f position, Vec2f size);
Button* createButton(const char* id, Vec2f position, Vec2f size, const char* label);
void destroyWidget(Widget* widget);

// Hierarchy management
void addChild(Widget* parent, Widget* child);
void removeChild(Widget* parent, Widget* child);

// Rendering
void drawWidget(Widget* widget);

// Input handling
void handleMouseClick(Widget* root, Vec2f position, int button, int action);
void handleMouseMove(Widget* root, Vec2f position);
void handleKeyPress(Widget* focused, int key, int scancode, int action, int mods);
void handleTextInput(Widget* focused, const char* text);

// Helper functions
Vec2f getAbsolutePosition(Widget* widget);
bool isInsideWidget(Widget* widget, Vec2f point);
Widget* findWidgetAtPosition(Widget* root, Vec2f point);
Widget* findFocusableWidgetAt(Widget* root, Vec2f point);

// Button-specific functions
void setButtonColors(Button* button, Color normal, Color hover, Color pressed);
void setButtonLabel(Button* button, const char* label);
/* void setButtonCallback(Button* button); // TODO */

void drawButton(Widget *widget);
void freeButton(Widget *widget);

#endif // WIDGETS_H
