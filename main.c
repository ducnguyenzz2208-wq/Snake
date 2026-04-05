#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CELL_SIZE 25

#define MENU_HEIGHT 50
#define GRID_WIDTH (SCREEN_WIDTH / CELL_SIZE)
#define GRID_HEIGHT ((SCREEN_HEIGHT - MENU_HEIGHT) / CELL_SIZE)

#define MAX_SNAKE_LENGTH (GRID_WIDTH * GRID_HEIGHT)

typedef struct Snake {
    Vector2 body[MAX_SNAKE_LENGTH];
    int length;
    Vector2 direction;
} Snake;

typedef struct Food {
    Vector2 position;
    bool active;
    Color color;
} Food;

// Global game state variables
Snake snake = { 0 };
Food food = { 0 };
bool gameOver = false;
int score = 0;
int framesCounter = 0;
int baseGameSpeed = 12; // Frames per update (lower is faster)
int gameSpeed = 12;

float timeAlive = 0.0f;

void InitGame(void);
void UpdateGame(void);
void DrawGame(void);

int main(void)
{
    // Enable Anti-Aliasing for smoother graphics
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Premium Snake Game - Raylib");
    SetTargetFPS(60);

    InitGame();

    while (!WindowShouldClose())
    {
        UpdateGame();
        DrawGame();
    }

    CloseWindow();
    return 0;
}

void SpawnFood(void) {
    bool validPosition = false;
    while (!validPosition) {
        food.position.x = GetRandomValue(0, GRID_WIDTH - 1);
        food.position.y = GetRandomValue(0, GRID_HEIGHT - 1);
        validPosition = true;
        
        // Ensure food doesn't spawn on the snake
        for (int i = 0; i < snake.length; i++) {
            if (food.position.x == snake.body[i].x && food.position.y == snake.body[i].y) {
                validPosition = false;
                break;
            }
        }
    }
    food.active = true;
    food.color = (Color){255, 50, 80, 255}; // Red/Pinkish apple
}

void InitGame(void) {
    snake.length = 4;
    snake.body[0] = (Vector2){ GRID_WIDTH / 2.0f, GRID_HEIGHT / 2.0f };
    snake.body[1] = (Vector2){ GRID_WIDTH / 2.0f - 1, GRID_HEIGHT / 2.0f };
    snake.body[2] = (Vector2){ GRID_WIDTH / 2.0f - 2, GRID_HEIGHT / 2.0f };
    snake.body[3] = (Vector2){ GRID_WIDTH / 2.0f - 3, GRID_HEIGHT / 2.0f };
    snake.direction = (Vector2){ 1, 0 }; // Moving right initially
    
    gameOver = false;
    score = 0;
    framesCounter = 0;
    timeAlive = 0.0f;
    gameSpeed = baseGameSpeed;
    
    SpawnFood();
}

void UpdateGame(void) {
    timeAlive += GetFrameTime();

    if (gameOver) {
        if (IsKeyPressed(KEY_ENTER)) InitGame();
        return;
    }

    // Input handling - prevent instant 180-degree turns
    if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && snake.direction.y != 1) {
        snake.direction = (Vector2){ 0, -1 };
    } else if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) && snake.direction.y != -1) {
        snake.direction = (Vector2){ 0, 1 };
    } else if ((IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) && snake.direction.x != 1) {
        snake.direction = (Vector2){ -1, 0 };
    } else if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) && snake.direction.x != -1) {
        snake.direction = (Vector2){ 1, 0 };
    }

    // Game logic update
    framesCounter++;
    if (framesCounter >= gameSpeed) {
        framesCounter = 0;

        // Move snake body
        for (int i = snake.length - 1; i > 0; i--) {
            snake.body[i] = snake.body[i - 1];
        }

        // Move snake head
        snake.body[0].x += snake.direction.x;
        snake.body[0].y += snake.direction.y;

        // Check Wall Collision
        if (snake.body[0].x < 0 || snake.body[0].x >= GRID_WIDTH || 
            snake.body[0].y < 0 || snake.body[0].y >= GRID_HEIGHT) {
            gameOver = true;
        }

        // Check Self Collision
        for (int i = 1; i < snake.length; i++) {
            if (snake.body[0].x == snake.body[i].x && snake.body[0].y == snake.body[i].y) {
                gameOver = true;
            }
        }

        // Check Food Collision
        if (snake.body[0].x == food.position.x && snake.body[0].y == food.position.y) {
            snake.length++;
            score += 10;
            // Slightly increase speed up to a limit
            if (score % 50 == 0 && gameSpeed > 4) {
                gameSpeed--; 
            }
            SpawnFood();
        }
    }
}



void DrawGame(void) {
    BeginDrawing();
    
    Color bgDark = (Color){20, 24, 34, 255}; // Deep navy background
    Color bgMenu = (Color){15, 18, 25, 255};
    Color gridColor = (Color){31, 38, 51, 230};
    
    ClearBackground(bgDark);
    
    // Draw Top Menu Bar
    DrawRectangle(0, 0, SCREEN_WIDTH, MENU_HEIGHT, bgMenu);
    DrawLine(0, MENU_HEIGHT, SCREEN_WIDTH, MENU_HEIGHT, (Color){50, 60, 80, 255});
    DrawTextEx(GetFontDefault(), TextFormat("SCORE: %04d", score), (Vector2){20, 15}, 22, 2, (Color){220, 220, 220, 255});
    DrawTextEx(GetFontDefault(), "PREMIUM SNAKE", (Vector2){SCREEN_WIDTH - 200, 15}, 20, 2, (Color){150, 160, 180, 255});

    // We draw game elements with a Y-offset to account for Menu
    Vector2 offset = {0, MENU_HEIGHT};

    if (!gameOver) {
        // Render Beautiful Grid
        for (int i = 0; i <= SCREEN_WIDTH / CELL_SIZE; i++) {
            DrawLineEx((Vector2){i * CELL_SIZE, offset.y}, (Vector2){i * CELL_SIZE, SCREEN_HEIGHT}, 1.0f, gridColor);
        }
        for (int i = 0; i <= GRID_HEIGHT; i++) {
            DrawLineEx((Vector2){0, offset.y + i * CELL_SIZE}, (Vector2){SCREEN_WIDTH, offset.y + i * CELL_SIZE}, 1.0f, gridColor);
        }

        // Render Food with pulsating glowing effect
        float pulse = (sinf(timeAlive * 6.0f) + 1.0f) / 2.0f; // 0.0 to 1.0
        float currentSize = CELL_SIZE * 0.4f + pulse * (CELL_SIZE * 0.15f);
        
        Vector2 foodCenter = { 
            food.position.x * CELL_SIZE + CELL_SIZE/2.0f, 
            food.position.y * CELL_SIZE + CELL_SIZE/2.0f + offset.y 
        };
        
        // Outer glow
        DrawCircleV(foodCenter, currentSize * 1.5f, Fade(food.color, 0.3f));
        // Inner core
        DrawCircleV(foodCenter, currentSize, food.color);
        // Small shiny highlight
        DrawCircleV((Vector2){foodCenter.x - currentSize*0.3f, foodCenter.y - currentSize*0.3f}, currentSize*0.3f, (Color){255, 200, 200, 200});

        // Render Snake
        Color headColor = (Color){40, 200, 150, 255};   // Cyan-Green
        Color tailColor = (Color){20, 110, 130, 255};   // Dark Blue-Green

        // Draw body starting from tail up to neck so head draws on top
        for (int i = snake.length - 1; i > 0; i--) {
            float t = (float)i / (snake.length - 1); // 0.0 = Head, 1.0 = Tail
            Color segColor = ColorLerp(headColor, tailColor, t);
            
            Rectangle rect = { 
                snake.body[i].x * CELL_SIZE + 2, 
                snake.body[i].y * CELL_SIZE + 2 + offset.y, 
                CELL_SIZE - 4, 
                CELL_SIZE - 4 
            };
            
            // Rounded rectangle for smoother body
            DrawRectangleRounded(rect, 0.5f, 4, segColor);
        }

        // Draw Head
        Rectangle headRect = { 
            snake.body[0].x * CELL_SIZE + 1, 
            snake.body[0].y * CELL_SIZE + 1 + offset.y, 
            CELL_SIZE - 2, 
            CELL_SIZE - 2 
        };
        // We draw the head slightly bulbous/larger
        DrawRectangleRounded(headRect, 0.4f, 8, (Color){60, 240, 170, 255}); // Brighter Head
        
        // Draw Eyes on Head based on direction
        float eyeOffsetX = 0.0f;
        float eyeOffsetY = 0.0f;
        
        if (snake.direction.x == 1) eyeOffsetX = 4.0f;        // Right
        else if (snake.direction.x == -1) eyeOffsetX = -4.0f; // Left
        else if (snake.direction.y == 1) eyeOffsetY = 4.0f;   // Down
        else if (snake.direction.y == -1) eyeOffsetY = -4.0f; // Up
        
        Vector2 headCenter = { headRect.x + headRect.width/2.0f, headRect.y + headRect.height/2.0f };
        
        Vector2 eye1 = headCenter;
        Vector2 eye2 = headCenter;
        
        // Position eyes perpendicular to movement
        if (snake.direction.x != 0) {
            eye1.y -= 5;
            eye2.y += 5;
            eye1.x += eyeOffsetX;
            eye2.x += eyeOffsetX;
        } else {
            eye1.x -= 5;
            eye2.x += 5;
            eye1.y += eyeOffsetY;
            eye2.y += eyeOffsetY;
        }

        // White of the eye
        DrawCircleV(eye1, 3.5f, WHITE);
        DrawCircleV(eye2, 3.5f, WHITE);
        // Pupils
        DrawCircleV((Vector2){eye1.x + snake.direction.x, eye1.y + snake.direction.y}, 1.5f, BLACK);
        DrawCircleV((Vector2){eye2.x + snake.direction.x, eye2.y + snake.direction.y}, 1.5f, BLACK);

    } else {
        // GAME OVER STATE
        const char* text1 = "GAME OVER";
        const char* text2 = "PRESS ENTER TO PLAY AGAIN";
        
        int w1 = MeasureText(text1, 60);
        int w2 = MeasureText(text2, 20);
        
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(bgDark, 0.85f));
        
        // Pulse effect for Game Over using sinf
        float pulse = (sinf(timeAlive * 3.0f) + 1.0f) / 2.0f;
        Color warnColor = ColorLerp((Color){230, 60, 60, 255}, (Color){255, 120, 120, 255}, pulse);

        DrawText(text1, SCREEN_WIDTH/2 - w1/2, SCREEN_HEIGHT/2 - 60, 60, warnColor);
        DrawText(text2, SCREEN_WIDTH/2 - w2/2, SCREEN_HEIGHT/2 + 20, 20, LIGHTGRAY);
    }

    EndDrawing();
}
