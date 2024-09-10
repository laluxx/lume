#include <lume.h>

// Screen dimensions
int sw = 1920;
int sh = 1080;

// Game objects structures
typedef struct {
    Vec2f position;
    Vec2f size;
    Color color;
} Paddle;

typedef struct {
    Vec2f position;
    float radius;
    Vec2f velocity;
    Color color;
} Ball;

typedef struct {
    Vec2f position;
    Vec2f size;
    bool isActive;
    Color color;
} Brick;

// Game constants
const int brickRows = 4;
const int brickColumns = 10;
Brick bricks[brickRows][brickColumns]; // Array of bricks

// Game objects
Paddle paddle;
Ball ball;

// Initialize game objects
void initGame() {
    // Initialize paddle
    paddle.position = (Vec2f){sw / 2 - 100, sh - 30};
    paddle.size = (Vec2f){200, 20};
    paddle.color = WHITE;

    // Initialize ball
    ball.position = (Vec2f){sw / 2, sh / 2};
    ball.radius = 10;
    ball.velocity = (Vec2f){-300, -300};
    ball.color = WHITE;

    // Initialize bricks
    int brickWidth = sw / brickColumns;
    int brickHeight = 30;
    for (int i = 0; i < brickRows; i++) {
        for (int j = 0; j < brickColumns; j++) {
            bricks[i][j].position = (Vec2f){j * brickWidth, i * brickHeight};
            bricks[i][j].size = (Vec2f){brickWidth - 5, brickHeight - 5};
            bricks[i][j].isActive = true;
            bricks[i][j].color = RED;
        }
    }
}

// Handle input
void updateInput() {
    if (isKeyDown(KEY_LEFT)) {
        paddle.position.x -= 500 * getFrameTime(); // Move left
    }
    if (isKeyDown(KEY_RIGHT)) {
        paddle.position.x += 500 * getFrameTime(); // Move right
    }
}

// Update game logic
void updateGame() {
    // Update ball position
    ball.position.x += ball.velocity.x * getFrameTime();
    ball.position.y += ball.velocity.y * getFrameTime();

    // Check for collision with walls
    if (ball.position.x < 0 || ball.position.x > sw) {
        ball.velocity.x = -ball.velocity.x;
    }
    if (ball.position.y < 0) {
        ball.velocity.y = -ball.velocity.y;
    }

    // Check for collision with the paddle
    if (ball.position.y > paddle.position.y - ball.radius &&
        ball.position.x > paddle.position.x &&
        ball.position.x < paddle.position.x + paddle.size.x) {
        ball.velocity.y = -ball.velocity.y;
    }

    // Check for collision with bricks
    for (int i = 0; i < brickRows; i++) {
        for (int j = 0; j < brickColumns; j++) {
            Brick *b = &bricks[i][j];
            if (b->isActive &&
                ball.position.x + ball.radius > b->position.x &&
                ball.position.x - ball.radius < b->position.x + b->size.x &&
                ball.position.y + ball.radius > b->position.y &&
                ball.position.y - ball.radius < b->position.y + b->size.y) {
                b->isActive = false;
                ball.velocity.y = -ball.velocity.y; // Reflect the ball
                break;
            }
        }
    }
}

// Draw game objects
void drawGame() {
    drawRectangle(paddle.position, paddle.size, paddle.color);
    drawCircle(ball.position, ball.radius, ball.color);

    for (int i = 0; i < brickRows; i++) {
        for (int j = 0; j < brickColumns; j++) {
            if (bricks[i][j].isActive) {
                drawRectangle(bricks[i][j].position, bricks[i][j].size, bricks[i][j].color);
            }
        }
    }
}

int main() {
    initWindow(sw, sh, "Breakout");
    initGame();

    while (!windowShouldClose()) {
        beginDrawing();
        clearBackground(BLACK);

        updateInput();
        updateGame();
        drawGame();

        endDrawing();
    }

    closeWindow();
    return 0;
}
