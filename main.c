#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CELL_SIZE 25

#define MENU_HEIGHT 50
#define GRID_WIDTH (SCREEN_WIDTH / CELL_SIZE)
#define GRID_HEIGHT ((SCREEN_HEIGHT - MENU_HEIGHT) / CELL_SIZE)

#define MAX_SNAKE_LENGTH (GRID_WIDTH * GRID_HEIGHT)

// State machine enum
typedef enum GameScreen {
  MENU,
  PLAYING,
  SETTINGS,
  GAME_OVER,
  PAUSE
} GameScreen;

typedef struct Snake {
  Vector2 body[MAX_SNAKE_LENGTH];
  int length;
  Vector2 direction;
  Vector2 nextDirection; // Buffer to prevent 180-turn bug
} Snake;

typedef struct Food {
  Vector2 position;
  bool active;
  Color color;
} Food;

#define MAX_BOMBS 100
typedef struct Bomb {
  Vector2 position;
  bool active;
} Bomb;

typedef struct SpecialFood {
  Vector2 position;
  bool active;
  float timer;
} SpecialFood;

// Global variables
GameScreen currentScreen = MENU;
Snake snake = {0};
Food food = {0};
Bomb bombs[MAX_BOMBS] = {0};
int numBombs = 0;
SpecialFood greenOrb = {0};
SpecialFood goldOrb = {0};
SpecialFood pinkOrb = {0};
int greenOrbTarget = 20;

Food frenzyFoods[20] = {0};
bool frenzyActive = false;
float frenzyTimer = 0.0f;
int normalFoodsEaten = 0;
int foodsSinceBombClear = 0;
int score = 0;
int framesCounter = 0;
int baseGameSpeed = 12; // Frames per update (lower is faster)
int gameSpeed = 12;
float timeAlive = 0.0f;
int difficulty = 1; // 0: Easy (16), 1: Normal (12), 2: Hard (8)

// Menu selections
int menuSelection = 0;     // 0: Play, 1: Settings
int settingsSelection = 0; // 0: Difficulty, 1: Back
int pauseSelection = 0;    // 0: Main Menu, 1: Quit Game
bool quitGame = false;

Music bgmMenu;
Music bgmEasy;
Music bgmNormal;
Music bgmHard;
Music *currentMusic = NULL;

void SwitchMusic(Music *newMusic) {
  if (currentMusic != newMusic) {
    if (currentMusic != NULL)
      StopMusicStream(*currentMusic);
    currentMusic = newMusic;
    if (currentMusic != NULL)
      PlayMusicStream(*currentMusic);
  }
}

void InitGame(void);
void UpdateGame(void);
void DrawGame(void);

int main(void) {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake");
  InitAudioDevice();

  bgmMenu = LoadMusicStream("main music.mp3");
  bgmEasy = LoadMusicStream("easy.mp3");
  bgmNormal = LoadMusicStream("normal.mp3");
  bgmHard = LoadMusicStream("hard.mp3");

  bgmMenu.looping = true;
  bgmEasy.looping = true;
  bgmNormal.looping = true;
  bgmHard.looping = true;

  currentMusic = &bgmMenu;
  PlayMusicStream(*currentMusic);

  SetTargetFPS(60);

  InitGame();

  while (!WindowShouldClose() && !quitGame) {
    UpdateGame();
    DrawGame();
  }

  UnloadMusicStream(bgmMenu);
  UnloadMusicStream(bgmEasy);
  UnloadMusicStream(bgmNormal);
  UnloadMusicStream(bgmHard);
  CloseAudioDevice();

  CloseWindow();
  return 0;
}

void SpawnBomb(void);
void SpawnGreenOrb(void);
void SpawnGoldOrb(void);
void SpawnPinkOrb(void);

Vector2 GetValidSpawnPosition(void) {
  Vector2 pos;
  bool valid = false;
  while (!valid) {
    pos.x = (float)GetRandomValue(0, GRID_WIDTH - 1);
    pos.y = (float)GetRandomValue(0, GRID_HEIGHT - 1);
    valid = true;
    for (int i = 0; i < snake.length; i++) {
      if (pos.x == snake.body[i].x && pos.y == snake.body[i].y) {
        valid = false;
        break;
      }
    }
    if (!valid)
      continue;
    if (food.active && pos.x == food.position.x && pos.y == food.position.y)
      valid = false;
    for (int i = 0; i < 20; i++)
      if (frenzyFoods[i].active && pos.x == frenzyFoods[i].position.x &&
          pos.y == frenzyFoods[i].position.y)
        valid = false;
    if (greenOrb.active && pos.x == greenOrb.position.x &&
        pos.y == greenOrb.position.y)
      valid = false;
    if (goldOrb.active && pos.x == goldOrb.position.x &&
        pos.y == goldOrb.position.y)
      valid = false;
    if (pinkOrb.active && pos.x == pinkOrb.position.x &&
        pos.y == pinkOrb.position.y)
      valid = false;
    for (int i = 0; i < numBombs; i++) {
      if (bombs[i].active && pos.x == bombs[i].position.x &&
          pos.y == bombs[i].position.y) {
        valid = false;
        break;
      }
    }
  }
  return pos;
}

void SpawnFood(void) {
  food.position = GetValidSpawnPosition();
  food.active = true;
  food.color = (Color){255, 50, 80, 255};
}

void SpawnBomb(void) {
  int r = GetRandomValue(1, 100);
  int count = 2;
  if (r > 70 && r <= 90)
    count = 3;
  if (r > 90)
    count = 4;

  for (int c = 0; c < count; c++) {
    if (numBombs >= MAX_BOMBS)
      return;
    bombs[numBombs].position = GetValidSpawnPosition();
    bombs[numBombs].active = true;
    numBombs++;
  }
}

void SpawnGreenOrb(void) {
  greenOrb.position = GetValidSpawnPosition();
  greenOrb.active = true;
  greenOrb.timer = 8.0f;
}

void SpawnGoldOrb(void) {
  goldOrb.position = GetValidSpawnPosition();
  goldOrb.active = true;
  goldOrb.timer = 5.0f;
}

void SpawnPinkOrb(void) {
  pinkOrb.position = GetValidSpawnPosition();
  pinkOrb.active = true;
  pinkOrb.timer = 6.0f;
}

void TriggerFrenzy(void) {
  frenzyActive = true;
  frenzyTimer = 7.0f; // 7 seconds
  numBombs = 0;       // Clear all bombs
  for (int i = 0; i < 20; i++) {
    frenzyFoods[i].position = GetValidSpawnPosition();
    frenzyFoods[i].active = true;
    frenzyFoods[i].color = (Color){255, 50, 80, 255};
  }
}

void InitGame(void) {
  snake.length = 4;
  snake.body[0] =
      (Vector2){(float)GRID_WIDTH / 2.0f, (float)GRID_HEIGHT / 2.0f};
  snake.body[1] =
      (Vector2){(float)GRID_WIDTH / 2.0f - 1.0f, (float)GRID_HEIGHT / 2.0f};
  snake.body[2] =
      (Vector2){(float)GRID_WIDTH / 2.0f - 2.0f, (float)GRID_HEIGHT / 2.0f};
  snake.body[3] =
      (Vector2){(float)GRID_WIDTH / 2.0f - 3.0f, (float)GRID_HEIGHT / 2.0f};
  snake.direction = (Vector2){1, 0};
  snake.nextDirection = (Vector2){1, 0};

  score = 0;
  framesCounter = 0;

  if (difficulty == 0)
    baseGameSpeed = 16;
  else if (difficulty == 1)
    baseGameSpeed = 12;
  else if (difficulty == 2)
    baseGameSpeed = 8;

  gameSpeed = baseGameSpeed;

  numBombs = 0;
  normalFoodsEaten = 0;
  foodsSinceBombClear = 0;
  greenOrb.active = false;
  goldOrb.active = false;
  pinkOrb.active = false;
  greenOrbTarget = GetRandomValue(20, 25);
  frenzyActive = false;
  frenzyTimer = 0.0f;
  for (int i = 0; i < 20; i++)
    frenzyFoods[i].active = false;
  SpawnFood();
}

void UpdateGame(void) {
  Music *targetMusic = &bgmMenu;
  if (currentScreen == PLAYING || currentScreen == PAUSE) {
    if (difficulty == 0)
      targetMusic = &bgmEasy;
    else if (difficulty == 1)
      targetMusic = &bgmNormal;
    else if (difficulty == 2)
      targetMusic = &bgmHard;
  }
  SwitchMusic(targetMusic);

  if (currentMusic != NULL) {
    UpdateMusicStream(*currentMusic);
  }

  float dt = GetFrameTime();
  timeAlive += dt;

  if (currentScreen == PLAYING) {
    if (greenOrb.active) {
      greenOrb.timer -= dt;
      if (greenOrb.timer <= 0)
        greenOrb.active = false;
    }
    if (goldOrb.active) {
      goldOrb.timer -= dt;
      if (goldOrb.timer <= 0)
        goldOrb.active = false;
    }
    if (pinkOrb.active) {
      pinkOrb.timer -= dt;
      if (pinkOrb.timer <= 0)
        pinkOrb.active = false;
    }
  }

  switch (currentScreen) {
  case MENU:
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      menuSelection++;
      if (menuSelection > 1)
        menuSelection = 0;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
      menuSelection--;
      if (menuSelection < 0)
        menuSelection = 1;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      if (menuSelection == 0) {
        InitGame();
        currentScreen = PLAYING;
      } else if (menuSelection == 1) {
        currentScreen = SETTINGS;
      }
    }
    break;

  case SETTINGS:
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      settingsSelection++;
      if (settingsSelection > 1)
        settingsSelection = 0;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
      settingsSelection--;
      if (settingsSelection < 0)
        settingsSelection = 1;
    }

    if (settingsSelection == 0) {
      if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        difficulty--;
        if (difficulty < 0)
          difficulty = 2;
      }
      if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        difficulty++;
        if (difficulty > 2)
          difficulty = 0;
      }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      if (settingsSelection == 1) {
        currentScreen = MENU;
      }
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
      currentScreen = MENU;
    }
    break;

  case PLAYING:
    // Input handling using nextDirection buffer
    if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) &&
        snake.direction.y != 1) {
      snake.nextDirection = (Vector2){0, -1};
    } else if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) &&
               snake.direction.y != -1) {
      snake.nextDirection = (Vector2){0, 1};
    } else if ((IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) &&
               snake.direction.x != 1) {
      snake.nextDirection = (Vector2){-1, 0};
    } else if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) &&
               snake.direction.x != -1) {
      snake.nextDirection = (Vector2){1, 0};
    }

    framesCounter++;
    if (framesCounter >= gameSpeed) {
      framesCounter = 0;
      snake.direction = snake.nextDirection;

      for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
      }

      snake.body[0].x += snake.direction.x;
      snake.body[0].y += snake.direction.y;

      // Wall Collision
      if (snake.body[0].x < 0 || snake.body[0].x >= GRID_WIDTH ||
          snake.body[0].y < 0 || snake.body[0].y >= GRID_HEIGHT) {
        currentScreen = GAME_OVER;
      }

      // Self Collision
      for (int i = 1; i < snake.length; i++) {
        if (snake.body[0].x == snake.body[i].x &&
            snake.body[0].y == snake.body[i].y) {
          currentScreen = GAME_OVER;
        }
      }

      // Green Orb Collision
      if (greenOrb.active && snake.body[0].x == greenOrb.position.x &&
          snake.body[0].y == greenOrb.position.y) {
        greenOrb.active = false;
        int newLength = (int)(snake.length * 0.7f);
        if (newLength < 4)
          newLength = 4;
        snake.length = newLength;
      }

      // Gold Orb Collision
      if (goldOrb.active && snake.body[0].x == goldOrb.position.x &&
          snake.body[0].y == goldOrb.position.y) {
        goldOrb.active = false;
        score += 50;
      }

      // Pink Orb Collision
      if (pinkOrb.active && snake.body[0].x == pinkOrb.position.x &&
          snake.body[0].y == pinkOrb.position.y) {
        pinkOrb.active = false;
        TriggerFrenzy();
      }

      // Bomb Collision
      for (int i = 0; i < numBombs; i++) {
        if (bombs[i].active && snake.body[0].x == bombs[i].position.x &&
            snake.body[0].y == bombs[i].position.y) {
          currentScreen = GAME_OVER;
        }
      }

      bool ateFood = false;

      // Regular Food Collision
      if (food.active && snake.body[0].x == food.position.x &&
          snake.body[0].y == food.position.y) {
        ateFood = true;
        SpawnFood();
      }

      // Frenzy Food Collision
      if (frenzyActive) {
        for (int i = 0; i < 20; i++) {
          if (frenzyFoods[i].active &&
              snake.body[0].x == frenzyFoods[i].position.x &&
              snake.body[0].y == frenzyFoods[i].position.y) {
            ateFood = true;
            // Respawn it immediately to keep screen full!
            frenzyFoods[i].position = GetValidSpawnPosition();
          }
        }
      }

      if (ateFood) {
        if (snake.length < MAX_SNAKE_LENGTH)
          snake.length++;
        score += 10;
        normalFoodsEaten++;

        // Randomly spawn special orbs when eating a food
        if (!goldOrb.active && GetRandomValue(1, 100) <= 10)
          SpawnGoldOrb(); // 10% chance
        if (!pinkOrb.active && GetRandomValue(1, 100) <= 12)
          SpawnPinkOrb(); // 12% chance

        if (!frenzyActive) {
          foodsSinceBombClear++;

          int bombSpawnFreq = 4; // default
          if (difficulty == 0)
            bombSpawnFreq = 5;
          else if (difficulty == 1)
            bombSpawnFreq = 4;
          else if (difficulty == 2)
            bombSpawnFreq = 3;

          if (score % (bombSpawnFreq * 10) == 0)
            SpawnBomb();

          if (normalFoodsEaten >= greenOrbTarget) {
            SpawnGreenOrb();
            normalFoodsEaten = 0;
            greenOrbTarget = GetRandomValue(20, 25);
          }

          if (foodsSinceBombClear >= 15) {
            numBombs = 0;
            foodsSinceBombClear = 0;
          }
        }

        if (score % 50 == 0 && gameSpeed > 4)
          gameSpeed--;
      }

      if (frenzyActive) {
        frenzyTimer -= (float)gameSpeed / 60.0f;
        if (frenzyTimer <= 0.0f) {
          frenzyActive = false;
          for (int i = 0; i < 20; i++)
            frenzyFoods[i].active = false;
        }
      }
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
      currentScreen = PAUSE;
      pauseSelection = 0;
    }
    break;

  case PAUSE:
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      pauseSelection++;
      if (pauseSelection > 2)
        pauseSelection = 0;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
      pauseSelection--;
      if (pauseSelection < 0)
        pauseSelection = 2;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      if (pauseSelection == 0) {
        currentScreen = PLAYING;
      } else if (pauseSelection == 1) {
        currentScreen = MENU;
      } else if (pauseSelection == 2) {
        quitGame = true;
      }
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
      currentScreen = PLAYING;
    }
    break;

  case GAME_OVER:
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      currentScreen = MENU;
    }
    break;
  }
}

void DrawGame(void) {
  BeginDrawing();

  Color bgDark = (Color){20, 24, 34, 255};
  Color bgMenu = (Color){15, 18, 25, 255};
  Color gridColor = (Color){31, 38, 51, 230};

  ClearBackground(bgDark);

  switch (currentScreen) {
  case MENU: {
    int titleWidth = MeasureText("Snake", 80);
    Color titleColor = (Color){60, 240, 170, 255};
    DrawText("Snake", SCREEN_WIDTH / 2 - titleWidth / 2, 100, 80, titleColor);

    const char *opt1 = "PLAY";
    const char *opt2 = "SETTINGS";

    int c1 = menuSelection == 0 ? 255 : 150;
    int c2 = menuSelection == 1 ? 255 : 150;

    DrawText(opt1, SCREEN_WIDTH / 2 - MeasureText(opt1, 40) / 2, 300, 40,
             (Color){c1, c1, c1, 255});
    DrawText(opt2, SCREEN_WIDTH / 2 - MeasureText(opt2, 40) / 2, 380, 40,
             (Color){c2, c2, c2, 255});

    float pulse = (sinf(timeAlive * 5.0f) + 1.0f) / 2.0f;
    if (menuSelection == 0)
      DrawText(">",
               SCREEN_WIDTH / 2 - MeasureText(opt1, 40) / 2 - 40 + pulse * 10,
               300, 40, titleColor);
    if (menuSelection == 1)
      DrawText(">",
               SCREEN_WIDTH / 2 - MeasureText(opt2, 40) / 2 - 40 + pulse * 10,
               380, 40, titleColor);
    break;
  }

  case SETTINGS: {
    int titleWidth = MeasureText("SETTINGS", 60);
    DrawText("SETTINGS", SCREEN_WIDTH / 2 - titleWidth / 2, 100, 60, LIGHTGRAY);

    const char *diffStr = "NORMAL";
    if (difficulty == 0)
      diffStr = "EASY";
    else if (difficulty == 2)
      diffStr = "HARD";

    const char *optDiff = TextFormat("DIFFICULTY:  < %s >", diffStr);
    const char *optBack = "BACK";

    int c1 = settingsSelection == 0 ? 255 : 150;
    int c2 = settingsSelection == 1 ? 255 : 150;

    DrawText(optDiff, SCREEN_WIDTH / 2 - MeasureText(optDiff, 30) / 2, 280, 30,
             (Color){c1, c1, c1, 255});
    DrawText(optBack, SCREEN_WIDTH / 2 - MeasureText(optBack, 30) / 2, 360, 30,
             (Color){c2, c2, c2, 255});

    float pulse = (sinf(timeAlive * 5.0f) + 1.0f) / 2.0f;
    Color highlight = (Color){60, 240, 170, 255};
    if (settingsSelection == 0)
      DrawText(">",
               SCREEN_WIDTH / 2 - MeasureText(optDiff, 30) / 2 - 30 + pulse * 5,
               280, 30, highlight);
    if (settingsSelection == 1)
      DrawText(">",
               SCREEN_WIDTH / 2 - MeasureText(optBack, 30) / 2 - 30 + pulse * 5,
               360, 30, highlight);

    break;
  }

  case PAUSE:
  case PLAYING: {
    DrawRectangle(0, 0, SCREEN_WIDTH, MENU_HEIGHT, bgMenu);
    DrawLine(0, MENU_HEIGHT, SCREEN_WIDTH, MENU_HEIGHT,
             (Color){50, 60, 80, 255});
    DrawTextEx(GetFontDefault(), TextFormat("SCORE: %04d", score),
               (Vector2){20, 15}, 22, 2, (Color){220, 220, 220, 255});
    DrawTextEx(GetFontDefault(), "Snake", (Vector2){SCREEN_WIDTH - 100, 15}, 20,
               2, (Color){150, 160, 180, 255});

    Vector2 offset = {0, MENU_HEIGHT};

    for (int i = 0; i <= SCREEN_WIDTH / CELL_SIZE; i++) {
      DrawLineEx((Vector2){i * CELL_SIZE, offset.y},
                 (Vector2){i * CELL_SIZE, SCREEN_HEIGHT}, 1.0f, gridColor);
    }
    for (int i = 0; i <= GRID_HEIGHT; i++) {
      DrawLineEx((Vector2){0, offset.y + i * CELL_SIZE},
                 (Vector2){SCREEN_WIDTH, offset.y + i * CELL_SIZE}, 1.0f,
                 gridColor);
    }

    float pulse = (sinf(timeAlive * 6.0f) + 1.0f) / 2.0f;
    float currentSize = CELL_SIZE * 0.4f + pulse * (CELL_SIZE * 0.15f);

    if (food.active) {
      Vector2 foodCenter = {food.position.x * CELL_SIZE + CELL_SIZE / 2.0f,
                            food.position.y * CELL_SIZE + CELL_SIZE / 2.0f +
                                offset.y};

      DrawCircleV(foodCenter, currentSize * 1.5f, Fade(food.color, 0.3f));
      DrawCircleV(foodCenter, currentSize, food.color);
      DrawCircleV((Vector2){foodCenter.x - currentSize * 0.3f,
                            foodCenter.y - currentSize * 0.3f},
                  currentSize * 0.3f, (Color){255, 200, 200, 200});
    }

    if (frenzyActive) {
      for (int i = 0; i < 20; i++) {
        if (frenzyFoods[i].active) {
          Vector2 fC = {frenzyFoods[i].position.x * CELL_SIZE +
                            CELL_SIZE / 2.0f,
                        frenzyFoods[i].position.y * CELL_SIZE +
                            CELL_SIZE / 2.0f + offset.y};
          DrawCircleV(fC, currentSize * 1.5f, Fade(frenzyFoods[i].color, 0.3f));
          DrawCircleV(fC, currentSize, frenzyFoods[i].color);
        }
      }
    }

    if (greenOrb.active) {
      Vector2 orbCenter = {greenOrb.position.x * CELL_SIZE + CELL_SIZE / 2.0f,
                           greenOrb.position.y * CELL_SIZE + CELL_SIZE / 2.0f +
                               offset.y};
      float orbPulse = (sinf(timeAlive * 10.0f) + 1.0f) / 2.0f;
      DrawCircleV(orbCenter, CELL_SIZE * 0.4f + orbPulse * (CELL_SIZE * 0.1f),
                  (Color){0, 255, 50, 255});
      DrawCircleV(orbCenter, CELL_SIZE * 0.2f, (Color){200, 255, 200, 255});
    }

    if (goldOrb.active) {
      Vector2 orbCenter = {goldOrb.position.x * CELL_SIZE + CELL_SIZE / 2.0f,
                           goldOrb.position.y * CELL_SIZE + CELL_SIZE / 2.0f +
                               offset.y};
      float orbPulse = (sinf(timeAlive * 12.0f) + 1.0f) / 2.0f;
      DrawCircleV(orbCenter, CELL_SIZE * 0.45f + orbPulse * 2.0f,
                  (Color){255, 215, 0, 150});
      DrawCircleV(orbCenter, CELL_SIZE * 0.35f, (Color){255, 223, 0, 255});
    }

    if (pinkOrb.active) {
      Vector2 orbCenter = {pinkOrb.position.x * CELL_SIZE + CELL_SIZE / 2.0f,
                           pinkOrb.position.y * CELL_SIZE + CELL_SIZE / 2.0f +
                               offset.y};
      float orbPulse = (sinf(timeAlive * 8.0f) + 1.0f) / 2.0f;
      DrawCircleV(orbCenter, CELL_SIZE * 0.4f + orbPulse * (CELL_SIZE * 0.1f),
                  (Color){255, 105, 180, 255});
      DrawCircleV(orbCenter, CELL_SIZE * 0.25f, (Color){255, 182, 193, 255});
    }

    for (int i = 0; i < numBombs; i++) {
      if (!bombs[i].active)
        continue;
      Vector2 bombCenter = {bombs[i].position.x * CELL_SIZE + CELL_SIZE / 2.0f,
                            bombs[i].position.y * CELL_SIZE + CELL_SIZE / 2.0f +
                                offset.y};
      float bombPulse = (sinf(timeAlive * 8.0f) + 1.0f) / 2.0f;

      DrawCircleV(bombCenter, CELL_SIZE * 0.35f, BLACK);
      for (int j = 0; j < 8; j++) {
        float angle = j * (3.14159f / 4.0f) + timeAlive * 2.0f;
        Vector2 p1 = {bombCenter.x + cosf(angle) * CELL_SIZE * 0.2f,
                      bombCenter.y + sinf(angle) * CELL_SIZE * 0.2f};
        Vector2 p2 = {bombCenter.x + cosf(angle) * CELL_SIZE * 0.5f,
                      bombCenter.y + sinf(angle) * CELL_SIZE * 0.5f};
        DrawLineEx(p1, p2, 2.0f, BLACK);
      }
      DrawCircleV(bombCenter, CELL_SIZE * 0.15f, (Color){255, 50, 50, 255});
      DrawCircleLines(bombCenter.x, bombCenter.y,
                      CELL_SIZE * 0.45f + bombPulse * 2.0f,
                      (Color){255, 50, 50, 150});
    }

    Color headColor = (Color){40, 200, 150, 255};
    Color tailColor = (Color){20, 110, 130, 255};

    for (int i = snake.length - 1; i > 0; i--) {
      float t = (float)i / (snake.length - 1);
      Color segColor = ColorLerp(headColor, tailColor, t);

      Rectangle rect = {snake.body[i].x * CELL_SIZE + 2,
                        snake.body[i].y * CELL_SIZE + 2 + offset.y,
                        CELL_SIZE - 4, CELL_SIZE - 4};

      DrawRectangleRounded(rect, 0.5f, 4, segColor);
    }

    Rectangle headRect = {snake.body[0].x * CELL_SIZE + 1,
                          snake.body[0].y * CELL_SIZE + 1 + offset.y,
                          CELL_SIZE - 2, CELL_SIZE - 2};
    DrawRectangleRounded(headRect, 0.4f, 8, (Color){60, 240, 170, 255});

    float eyeOffsetX = 0.0f;
    float eyeOffsetY = 0.0f;

    if (snake.direction.x == 1)
      eyeOffsetX = 4.0f;
    else if (snake.direction.x == -1)
      eyeOffsetX = -4.0f;
    else if (snake.direction.y == 1)
      eyeOffsetY = 4.0f;
    else if (snake.direction.y == -1)
      eyeOffsetY = -4.0f;

    Vector2 headCenter = {headRect.x + headRect.width / 2.0f,
                          headRect.y + headRect.height / 2.0f};

    Vector2 eye1 = headCenter;
    Vector2 eye2 = headCenter;

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

    DrawCircleV(eye1, 3.5f, WHITE);
    DrawCircleV(eye2, 3.5f, WHITE);
    DrawCircleV(
        (Vector2){eye1.x + snake.direction.x, eye1.y + snake.direction.y}, 1.5f,
        BLACK);
    DrawCircleV(
        (Vector2){eye2.x + snake.direction.x, eye2.y + snake.direction.y}, 1.5f,
        BLACK);

    if (currentScreen == PAUSE) {
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(bgDark, 0.85f));
      DrawText("PAUSED", SCREEN_WIDTH / 2 - MeasureText("PAUSED", 60) / 2,
               SCREEN_HEIGHT / 2 - 100, 60, (Color){200, 220, 255, 255});
      int c1 = pauseSelection == 0 ? 255 : 150;
      int c2 = pauseSelection == 1 ? 255 : 150;
      int c3 = pauseSelection == 2 ? 255 : 150;
      DrawText("CONTINUE", SCREEN_WIDTH / 2 - MeasureText("CONTINUE", 30) / 2,
               SCREEN_HEIGHT / 2 - 20, 30, (Color){c1, c1, c1, 255});
      DrawText("MAIN MENU", SCREEN_WIDTH / 2 - MeasureText("MAIN MENU", 30) / 2,
               SCREEN_HEIGHT / 2 + 30, 30, (Color){c2, c2, c2, 255});
      DrawText("QUIT GAME", SCREEN_WIDTH / 2 - MeasureText("QUIT GAME", 30) / 2,
               SCREEN_HEIGHT / 2 + 80, 30, (Color){c3, c3, c3, 255});
      float pulsePause = (sinf(timeAlive * 5.0f) + 1.0f) / 2.0f;
      Color highlight = (Color){60, 240, 170, 255};
      if (pauseSelection == 0)
        DrawText(">",
                 SCREEN_WIDTH / 2 - MeasureText("CONTINUE", 30) / 2 - 30 +
                     pulsePause * 5,
                 SCREEN_HEIGHT / 2 - 20, 30, highlight);
      if (pauseSelection == 1)
        DrawText(">",
                 SCREEN_WIDTH / 2 - MeasureText("MAIN MENU", 30) / 2 - 30 +
                     pulsePause * 5,
                 SCREEN_HEIGHT / 2 + 30, 30, highlight);
      if (pauseSelection == 2)
        DrawText(">",
                 SCREEN_WIDTH / 2 - MeasureText("QUIT GAME", 30) / 2 - 30 +
                     pulsePause * 5,
                 SCREEN_HEIGHT / 2 + 80, 30, highlight);
    }

    break;
  }

  case GAME_OVER: {
    const char *text1 = "GAME OVER";
    const char *text2 = "PRESS ENTER TO RETURN TO MENU";

    const char *flavorText;
    if (score < 500)
      flavorText = "NOOB! TRY HARDER!";
    else if (score < 1500)
      flavorText = "NOT BAD, BUT KEEP GOING!";
    else if (score < 3000)
      flavorText = "GOOD JOB! YOU HAVE POTENTIAL!";
    else if (score < 5000)
      flavorText = "AWESOME! YOU ARE A SNAKE MASTER!";
    else
      flavorText = "LEGENDARY! YOU ARE UNSTOPPABLE!";

    int w1 = MeasureText(text1, 60);
    int w2 = MeasureText(text2, 20);
    int w3 = MeasureText(flavorText, 25);

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(bgDark, 0.85f));

    float pulse = (sinf(timeAlive * 3.0f) + 1.0f) / 2.0f;
    Color warnColor = ColorLerp((Color){230, 60, 60, 255},
                                (Color){255, 120, 120, 255}, pulse);

    DrawText(text1, SCREEN_WIDTH / 2 - w1 / 2, SCREEN_HEIGHT / 2 - 80, 60,
             warnColor);
    DrawText(flavorText, SCREEN_WIDTH / 2 - w3 / 2, SCREEN_HEIGHT / 2, 25,
             (Color){200, 220, 255, 255});
    DrawText(text2, SCREEN_WIDTH / 2 - w2 / 2, SCREEN_HEIGHT / 2 + 60, 20,
             LIGHTGRAY);
    break;
  }
  }

  EndDrawing();
}
