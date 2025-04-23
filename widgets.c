#include "widgets.h"
#include "font.h"
#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

Font *widgetsFont;

// Helper function to duplicate a string
char* ourstrdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* new = malloc(len);
    if (new) memcpy(new, s, len);
    return new;
}

// Core widget functions
Widget* createWidget(const char* id, Vec2f position, Vec2f size) {
    Widget* widget = (Widget*)malloc(sizeof(Widget));
    if (!widget) return NULL;

    widget->id = ourstrdup(id);
    widget->visible = true;
    widget->active = true;
    widget->alpha = 1.0f;
    
    widget->parent = NULL;
    widget->firstChild = NULL;
    widget->lastChild = NULL;
    widget->nextSibling = NULL;
    widget->prevSibling = NULL;
    
    widget->position = position;
    widget->size = size;
    widget->minSize = (Vec2f){0, 0};
    widget->maxSize = (Vec2f){INFINITY, INFINITY};
    
    widget->bgColor = (Color){200, 200, 200, 1.0f};
    widget->fgColor = (Color){0, 0, 0, 1.0f};
    widget->borderColor = (Color){100, 100, 100, 1.0f};
    widget->borderWidth = 0.0f;
    widget->cornerRadius = 0.0f;
    
    widget->isHovered = false;
    widget->isDown = false;
    
    widget->onClick = NULL;
    widget->onTextInput = NULL;
    
    widget->draw = NULL;
    widget->update = NULL;
    widget->free = NULL;
    widget->layout = NULL;
    
    return widget;
}

Div* createDiv(const char* id, Vec2f position, Vec2f size) {
    Div* div = (Div*)malloc(sizeof(Div));
    if (!div) return NULL;
    
    Widget* base = createWidget(id, position, size);
    if (!base) {
        free(div);
        return NULL;
    }
    
    memcpy(&div->base, base, sizeof(Widget));
    free(base);
    
    div->scroll = (Vec2f){0, 0};
    
    // Set Div-specific functions
    div->base.draw = drawWidget;
    div->base.free = (void (*)(Widget*))free;
    
    return div;
}

Button* createButton(const char* id, Vec2f position, Vec2f size, const char* label) {
    Button* button = (Button*)malloc(sizeof(Button));
    if (!button) return NULL;
    
    Widget* base = createWidget(id, position, size);
    if (!base) {
        free(button);
        return NULL;
    }
    
    memcpy(&button->base, base, sizeof(Widget));
    free(base);
    
    button->label = ourstrdup(label);
    // TODO Inherit from a wtheme
    button->normalColor = (Color){0, 0, 1, 1.0f};
    button->hoverColor = (Color){1, 1, 0, 1.0f};
    button->pressedColor = (Color){1, 0, 0, 1.0f};
    button->textColor = (Color){1, 1, 1, 1.0f};
    button->fontSize = 16.0f;
    
    // Set Button-specific functions
    button->base.draw = drawButton;
    button->base.free = (void (*)(Widget*))freeButton;
    
    return button;
}

void destroyWidget(Widget* widget) {
    if (!widget) return;
    
    // Free all children
    Widget* child = widget->firstChild;
    while (child) {
        Widget* next = child->nextSibling;
        destroyWidget(child);
        child = next;
    }
    
    // Call widget-specific free function if it exists
    if (widget->free) {
        widget->free(widget);
    } else {
        // Default cleanup
        free(widget->id);
        free(widget);
    }
}

// Hierarchy management
void addChild(Widget* parent, Widget* child) {
    if (!parent || !child) return;
    
    // Remove child from any existing parent
    if (child->parent) {
        removeChild(child->parent, child);
    }
    
    child->parent = parent;
    
    if (!parent->firstChild) {
        // First child
        parent->firstChild = child;
        parent->lastChild = child;
        child->prevSibling = NULL;
        child->nextSibling = NULL;
    } else {
        // Append to end of list
        child->prevSibling = parent->lastChild;
        parent->lastChild->nextSibling = child;
        parent->lastChild = child;
        child->nextSibling = NULL;
    }
}

void removeChild(Widget* parent, Widget* child) {
    if (!parent || !child || child->parent != parent) return;
    
    if (child->prevSibling) {
        child->prevSibling->nextSibling = child->nextSibling;
    } else {
        parent->firstChild = child->nextSibling;
    }
    
    if (child->nextSibling) {
        child->nextSibling->prevSibling = child->prevSibling;
    } else {
        parent->lastChild = child->prevSibling;
    }
    
    child->parent = NULL;
    child->prevSibling = NULL;
    child->nextSibling = NULL;
}

// Rendering
void drawWidget(Widget* widget) {
    if (!widget || !widget->visible) return;
    
    Vec2f absPos = getAbsolutePosition(widget);
    
    // Draw background
    if (widget->bgColor.a > 0.0f) {
        drawRectangle(absPos, widget->size, widget->bgColor);
    }
    
    // Draw border
    if (widget->borderWidth > 0.0f && widget->borderColor.a > 0.0f) {
        drawRectangleLines(absPos, widget->size, widget->borderColor, widget->borderWidth);
    }
    
    // Draw children
    Widget* child = widget->firstChild;
    while (child) {
        child->draw(child);
        child = child->nextSibling;
    }
}



void drawButton(Widget *widget) {
    Button *button = (Button *)widget;
    if (!widget->visible)
        return;

    Vec2f absPos = getAbsolutePosition(widget);

    // Determine button color based on state
    Color bgColor;
    if (widget->isDown) {
        bgColor = button->pressedColor;
    } else if (widget->isHovered) {
        bgColor = button->hoverColor;
    } else {
        bgColor = button->normalColor;
    }

    useShader("simple");
    // Draw button background
    drawRectangle(absPos, widget->size, bgColor);

    // Draw button border if specified
    if (widget->borderWidth > 0.0f) {
        drawRectangleLines(absPos, widget->size, widget->borderColor,
                           widget->borderWidth);
    }
    flush();

    // Draw button label using the global widgetsFont
    if (button->label && button->label[0] != '\0') {
        useShader("text");
        
        // Get text dimensions (without scaling)
        float textWidth = getTextWidth(widgetsFont, button->label);
        float textHeight = getFontHeight(widgetsFont);

        // Calculate centered text position
        Vec2f textPos = {
            absPos.x + (widget->size.x - textWidth) * 0.5f,
            absPos.y + (widget->size.y - textHeight) * 0.5f + 
            (textHeight - widgetsFont->descent) * 0.5f  // Adjust for baseline
        };

        // Draw the text without scaling
        drawText(widgetsFont, button->label, textPos.x, textPos.y, button->textColor);
    }
    flush();

    // Draw children (if any)
    Widget *child = widget->firstChild;
    while (child) {
        child->draw(child);
        child = child->nextSibling;
    }
}

void freeButton(Widget* widget) {
    Button* button = (Button*)widget;
    if (button->label) free(button->label);
    free(button);
}

// Input handling
void handleMouseClick(Widget* root, Vec2f position, int button, int action) {
    if (!root) return;
    
    Widget* target = findWidgetAtPosition(root, position);
    if (!target) return;
    
    if (button == 0) { // Left mouse button
        if (action == 1) { // Press
            target->isDown = true;
        } else if (action == 0) { // Release
            if (target->isDown && target->onClick) {
                target->onClick(target);
            }
            target->isDown = false;
        }
    }
}

void handleMouseMove(Widget* root, Vec2f position) {
    if (!root) return;
    
    // First clear all hover states
    Widget* widget = root;
    while (widget) {
        widget->isHovered = false;
        widget = widget->nextSibling; // Only works for flat hierarchy - would need recursion for full tree
    }
    
    // Set hover state for widget under cursor
    Widget* target = findWidgetAtPosition(root, position);
    if (target) {
        target->isHovered = true;
    }
}

void handleKeyPress(Widget* focused, int key, int scancode, int action, int mods) {
    if (!focused) return;
    // TODO: Implement key handling for focused widget
}

void handleTextInput(Widget* focused, const char* text) {
    if (!focused || !focused->onTextInput) return;
    focused->onTextInput(focused, text);
}

// Helper functions
Vec2f getAbsolutePosition(Widget* widget) {
    Vec2f pos = widget->position;
    Widget* parent = widget->parent;
    while (parent) {
        pos.x += parent->position.x;
        pos.y += parent->position.y;
        parent = parent->parent;
    }
    return pos;
}

bool isInsideWidget(Widget* widget, Vec2f point) {
    if (!widget || !widget->visible) return false;
    
    Vec2f absPos = getAbsolutePosition(widget);
    return (point.x >= absPos.x && 
            point.x <= absPos.x + widget->size.x && 
            point.y >= absPos.y && 
            point.y <= absPos.y + widget->size.y);
}

Widget* findWidgetAtPosition(Widget* root, Vec2f point) {
    if (!root || !root->visible) return NULL;
    
    // Check children first (top-most widgets are last in the list)
    Widget* child = root->lastChild;
    while (child) {
        Widget* found = findWidgetAtPosition(child, point);
        if (found) return found;
        child = child->prevSibling;
    }
    
    // Check this widget
    if (isInsideWidget(root, point)) {
        return root;
    }
    
    return NULL;
}

Widget* findFocusableWidgetAt(Widget* root, Vec2f point) {
    Widget* widget = findWidgetAtPosition(root, point);
    while (widget && !widget->active) {
        widget = widget->parent;
    }
    return widget;
}

// Button-specific functions
void setButtonColors(Button* button, Color normal, Color hover, Color pressed) {
    if (!button) return;
    button->normalColor = normal;
    button->hoverColor = hover;
    button->pressedColor = pressed;
}

void setButtonLabel(Button* button, const char* label) {
    if (!button) return;
    if (button->label) free(button->label);
    button->label = ourstrdup(label);
}

 
