#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CELL_SIZE 25

#define MENU_HEIGHT 50
#define BORDER_MARGIN 25
#define GRID_WIDTH ((SCREEN_WIDTH - 2 * BORDER_MARGIN) / CELL_SIZE)
#define GRID_HEIGHT ((SCREEN_HEIGHT - MENU_HEIGHT - 2 * BORDER_MARGIN) / CELL_SIZE)

#define MAX_SNAKE_LENGTH (GRID_WIDTH * GRID_HEIGHT)

// State machine enum
typedef enum GameScreen { MENU, PLAYING, SETTINGS, GAME_OVER, PAUSE } GameScreen;

#define MAX_INPUT_QUEUE 3

typedef struct Snake {
    Vector2 body[MAX_SNAKE_LENGTH];
    int length;
    Vector2 direction;
    Vector2 inputQueue[MAX_INPUT_QUEUE];
    int inputCount;
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

#define MAX_STARS 100
typedef struct Star {
    Vector2 position;
    float size;
    float phase;
    Color color;
} Star;
Star backgroundStars[MAX_STARS];

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

// === BIẾN CHO MÀN HÌNH SETTINGS ===
Texture2D arrowLeftNormal, arrowLeftHover;
Texture2D arrowRightNormal, arrowRightHover;
Texture2D btnBackNormal, btnBackHover;

Rectangle arrowLeftBounds;
Rectangle arrowRightBounds;
Rectangle btnBackBounds;

bool isLeftHovered = false;
bool isRightHovered = false;
bool isBackHovered = false;

// === BIẾN CHO GRID ===
bool showGrid = true;
Rectangle gridArrowLeftBounds;
Rectangle gridArrowRightBounds;
bool isGridLeftHovered = false;
bool isGridRightHovered = false;

int moveset = 0; // 0: ARROWS, 1: WASD
Rectangle movesetArrowLeftBounds;
Rectangle movesetArrowRightBounds;
bool isMovesetLeftHovered = false;
bool isMovesetRightHovered = false;

// === BIẾN CHO SKIN ===
int snakeSkin = 0; // 0: DEFAULT, 1: GREEN, 2: BLUE, 3: RED, 4: GOLD
Rectangle skinArrowLeftBounds;
Rectangle skinArrowRightBounds;
bool isSkinLeftHovered = false;
bool isSkinRightHovered = false;

// === THÊM BIẾN CHO NÚT BẤM VÀO ĐÂY ===
Texture2D btnStartNormal;
Texture2D btnStartHover;
Rectangle btnStartBounds;
bool isStartHovered = false;
// === BIẾN NÚT EXIT CHO GAME OVER ===
Texture2D btnExitNormal;
Texture2D btnExitHover;
Rectangle btnExitBounds;
bool isExitHovered = false;
// === BIẾN NÚT EXIT CHO MENU ===
Texture2D btnMenuExitNormal;
Texture2D btnMenuExitHover;
Rectangle btnMenuExitBounds;
bool isMenuExitHovered = false;
// === THÊM BIẾN CHO NÚT SETTINGS ===
Texture2D btnSettingsNormal;
Texture2D btnSettingsHover;
Rectangle btnSettingsBounds;
bool isSettingsHovered = false;
// ==================================
// === BIẾN CHO RẮN VÀ THỨC ĂN ===
Texture2D texHead;
Texture2D texBody;
Texture2D texFood;
Texture2D texPinkOrb;
Texture2D texGoldOrb;
Texture2D texGreenOrb;
Texture2D texTrophy;
Texture2D texBomb;
Texture2D texBorder;

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

// === VOLUME CONTROL ===
float globalVolume = 1.0f;
Rectangle volMinusBounds;
Rectangle volPlusBounds;
bool isVolMinusHovered = false;
bool isVolPlusHovered = false;
Texture2D texSoundOn;
Texture2D texSoundOff;
bool isMuted = false;
Rectangle btnMuteBounds;
bool isMuteHovered = false;

// === HIGH SCORES ===
int topScores[3] = {0, 0, 0};
bool showHighScores = false;
Rectangle btnHighScoresBounds;
bool isHighScoresHovered = false;

// === FALLING ICONS BACKGROUND ===
typedef struct {
    Vector2 position;
    float speedY;
    float rotation;
    float rotationSpeed;
    float scale;
    int texType;
} FallingIcon;

#define MAX_FALLING_ICONS 30
FallingIcon fallingIcons[MAX_FALLING_ICONS];

void InitFallingIcons(void) {
    for (int i = 0; i < MAX_FALLING_ICONS; i++) {
        fallingIcons[i].position.x = GetRandomValue(0, SCREEN_WIDTH);
        fallingIcons[i].position.y = GetRandomValue(-SCREEN_HEIGHT, SCREEN_HEIGHT);
        fallingIcons[i].speedY = GetRandomValue(20, 60);
        fallingIcons[i].rotation = GetRandomValue(0, 360);
        fallingIcons[i].rotationSpeed = GetRandomValue(-45, 45);
        fallingIcons[i].scale = GetRandomValue(30, 60) / 10.0f;
        fallingIcons[i].texType = GetRandomValue(0, 4);
    }
}

void UpdateFallingIcons(float dt) {
    for (int i = 0; i < MAX_FALLING_ICONS; i++) {
        fallingIcons[i].position.y += fallingIcons[i].speedY * dt;
        fallingIcons[i].rotation += fallingIcons[i].rotationSpeed * dt;

        if (fallingIcons[i].position.y > SCREEN_HEIGHT + 100) {
            fallingIcons[i].position.x = GetRandomValue(0, SCREEN_WIDTH);
            fallingIcons[i].position.y = GetRandomValue(-200, -50);
            fallingIcons[i].speedY = GetRandomValue(20, 60);
            fallingIcons[i].rotationSpeed = GetRandomValue(-45, 45);
            fallingIcons[i].scale = GetRandomValue(30, 60) / 10.0f;
            fallingIcons[i].texType = GetRandomValue(0, 4);
        }
    }
}

void DrawFallingIcons(void) {
    Color tint = (Color){255, 255, 255, 40}; // Màu hơi tối, trong suốt
    for (int i = 0; i < MAX_FALLING_ICONS; i++) {
        Texture2D tex;
        float actualScale = fallingIcons[i].scale;
        switch(fallingIcons[i].texType) {
            case 0: tex = texFood; break;
            case 1: tex = texGreenOrb; break;
            case 2: tex = texPinkOrb; break;
            case 3: tex = texGoldOrb; break;
            case 4: 
                tex = texBomb; 
                actualScale = fallingIcons[i].scale * (8.0f / 350.0f); // Chuẩn hóa bomb
                break;
            default: tex = texFood; break;
        }
        
        Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
        Rectangle dest = {fallingIcons[i].position.x, fallingIcons[i].position.y, tex.width * actualScale, tex.height * actualScale};
        Vector2 origin = {dest.width/2.0f, dest.height/2.0f}; 
        
        DrawTexturePro(tex, source, dest, origin, fallingIcons[i].rotation, tint);
    }
}

void LoadScores(void) {
    FILE *fp = fopen("scores.txt", "r");
    if (fp) {
        fscanf(fp, "%d %d %d", &topScores[0], &topScores[1], &topScores[2]);
        fclose(fp);
    }
}

void SaveScores(void) {
    FILE *fp = fopen("scores.txt", "w");
    if (fp) {
        fprintf(fp, "%d %d %d", topScores[0], topScores[1], topScores[2]);
        fclose(fp);
    }
}

void UpdateScores(int newScore) {
    if (newScore > topScores[0]) {
        topScores[2] = topScores[1];
        topScores[1] = topScores[0];
        topScores[0] = newScore;
        SaveScores();
    } else if (newScore > topScores[1]) {
        topScores[2] = topScores[1];
        topScores[1] = newScore;
        SaveScores();
    } else if (newScore > topScores[2]) {
        topScores[2] = newScore;
        SaveScores();
    }
}

void TriggerGameOver(void) {
    UpdateScores(score);
    currentScreen = GAME_OVER;
}

Music bgmMenu;
Music bgmEasy;
Music bgmNormal;
Music bgmHard;
Music bgmLose;
Music *currentMusic = NULL;

void SwitchMusic(Music *newMusic) {
    if (currentMusic != newMusic) {
        if (currentMusic != NULL) StopMusicStream(*currentMusic);
        currentMusic = newMusic;
        if (currentMusic != NULL) PlayMusicStream(*currentMusic);
    }
}

// === THÊM HÀM CHUYỂN ĐỔI TOẠ ĐỘ CHUỘT ẢO ===
Vector2 GetVirtualMousePosition(void) {
    Vector2 mouse = GetMousePosition();
    float scale = fminf((float)GetScreenWidth() / SCREEN_WIDTH, (float)GetScreenHeight() / SCREEN_HEIGHT);
    
    Vector2 offset = {
        (GetScreenWidth() - (SCREEN_WIDTH * scale)) * 0.5f,
        (GetScreenHeight() - (SCREEN_HEIGHT * scale)) * 0.5f
    };
    
    Vector2 virtualMouse = {
        (mouse.x - offset.x) / scale,
        (mouse.y - offset.y) / scale
    };
    
    return virtualMouse;
}

void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void SpawnBomb(void);
void SpawnGreenOrb(void);
void SpawnGoldOrb(void);
void SpawnPinkOrb(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake");
    InitAudioDevice();

    // Khởi tạo nền sao lấp lánh cho Grid
    for (int i = 0; i < MAX_STARS; i++) {
        backgroundStars[i].position = (Vector2){
            (float)GetRandomValue(0, GRID_WIDTH * CELL_SIZE),
            (float)GetRandomValue(0, GRID_HEIGHT * CELL_SIZE)
        };
        backgroundStars[i].size = (float)GetRandomValue(1, 3);
        backgroundStars[i].phase = (float)GetRandomValue(0, 628) / 100.0f; // 0 to 6.28
        int brightness = GetRandomValue(150, 255);
        backgroundStars[i].color = (Color){brightness, brightness, brightness, 150};
    }

    bgmMenu = LoadMusicStream("main music.mp3");
    bgmEasy = LoadMusicStream("easy.mp3");
    bgmNormal = LoadMusicStream("normal.mp3");
    bgmHard = LoadMusicStream("hard.mp3");
    bgmLose = LoadMusicStream("lose sound.mp3");

    bgmMenu.looping = true;
    bgmEasy.looping = true;
    bgmNormal.looping = true;
    bgmHard.looping = true;
    bgmLose.looping = false; // Phát 1 lần khi Game Over

    currentMusic = &bgmMenu;
    PlayMusicStream(*currentMusic);

    SetTargetFPS(60);

    InitFallingIcons();

    arrowLeftNormal = LoadTexture("assets/arrow_left.png");
    arrowLeftHover = LoadTexture("assets/arrow_left_hover.png");
    arrowRightNormal = LoadTexture("assets/arrow_right.png");
    arrowRightHover = LoadTexture("assets/arrow_right_hover.png");
    btnBackNormal = LoadTexture("assets/backward_normal.png");
    btnBackHover = LoadTexture("assets/backward_pressed.png");

    // Định vị: Lấy tọa độ trung tâm màn hình để căn lề
    float centerX = SCREEN_WIDTH / 2.0f;

    // Mũi tên trái: cách tâm 180px về bên trái
    arrowLeftBounds = (Rectangle){ centerX + 25, 196, 48, 48 };
    // Mũi tên phải: cách tâm 150px về bên phải
    arrowRightBounds = (Rectangle){ centerX + 180, 196, 48, 48 };
    // Mũi tên cho Grid
    gridArrowLeftBounds = (Rectangle){ centerX + 25, 251, 48, 48 };
    gridArrowRightBounds = (Rectangle){ centerX + 180, 251, 48, 48 };
    
    // Mũi tên cho Moveset
    movesetArrowLeftBounds = (Rectangle){ centerX + 25, 306, 48, 48 };
    movesetArrowRightBounds = (Rectangle){ centerX + 180, 306, 48, 48 };

    // Mũi tên cho Skin
    skinArrowLeftBounds = (Rectangle){ centerX + 25, 361, 48, 48 };
    skinArrowRightBounds = (Rectangle){ centerX + 180, 361, 48, 48 };

    // Nút Back: ở dưới cùng
    btnBackBounds = (Rectangle){ centerX - 50, 430, 100, 40 };

    volMinusBounds = (Rectangle){SCREEN_WIDTH - 230, 20, 30, 30};
    volPlusBounds = (Rectangle){SCREEN_WIDTH - 40, 20, 30, 30};
    
    // Nút góc trái dưới cho Top 3 (Hình chiếc cúp)
    btnHighScoresBounds = (Rectangle){20, SCREEN_HEIGHT - 80, 60, 60};
    
    // Nút góc phải dưới cho Exit Menu
    btnMenuExitBounds = (Rectangle){SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80, 60, 60};
    
    // Nút tắt mở âm thanh góc trên phải trong Settings (Lớn hơn)
    btnMuteBounds = (Rectangle){SCREEN_WIDTH - 80, 20, 60, 60};

    // === THÊM VÙNG 1: TẢI ẢNH VÀ ĐỊNH VỊ TRÍ ===
    btnStartNormal = LoadTexture("assets/Start.png");
    btnStartHover = LoadTexture("assets/start_pressed.png");
    // Tải ảnh rắn và thức ăn (chỉ cần file ảnh 8x8)
    texHead = LoadTexture("assets/head.png");
    texBody = LoadTexture("assets/body.png");
    texFood = LoadTexture("assets/food.png");
    texPinkOrb = LoadTexture("assets/pink_orb.png");
    texGoldOrb = LoadTexture("assets/gold_orb.png");
    texGreenOrb = LoadTexture("assets/green_orb.png");
    texSoundOn = LoadTexture("assets/sound_on.png");
    texSoundOff = LoadTexture("assets/sound_off.png");
    texBorder = LoadTexture("assets/border.png");
    texTrophy = LoadTexture("assets/trophy.png");
    texBomb = LoadTexture("assets/bomb.png");
    // LỆNH QUAN TRỌNG: Bắt Raylib phóng to sắc nét (không bị mờ)
    SetTextureFilter(texHead, TEXTURE_FILTER_POINT);
    SetTextureFilter(texBody, TEXTURE_FILTER_POINT);
    SetTextureFilter(texFood, TEXTURE_FILTER_POINT);
    SetTextureFilter(texSoundOn, TEXTURE_FILTER_POINT);
    SetTextureFilter(texSoundOff, TEXTURE_FILTER_POINT);
    SetTextureFilter(texBorder, TEXTURE_FILTER_POINT);
    SetTextureFilter(texPinkOrb, TEXTURE_FILTER_POINT);
    SetTextureFilter(texGoldOrb, TEXTURE_FILTER_POINT);
    SetTextureFilter(texGreenOrb, TEXTURE_FILTER_POINT);
    SetTextureFilter(texTrophy, TEXTURE_FILTER_POINT);
    SetTextureFilter(texBomb, TEXTURE_FILTER_POINT);
    // === THÊM TẢI ẢNH SETTINGS VÀO ĐÂY ===
    btnSettingsNormal = LoadTexture("assets/BTN_SETTINGS.png");
    btnSettingsHover = LoadTexture("assets/BTN_SETTINGS_PRESSED.png");
    
    btnExitNormal = LoadTexture("assets/BTN_EXIT_2.png");
    btnExitHover = LoadTexture("assets/BTN_EXIT_PRESSED_1.png");
    btnExitBounds = (Rectangle){(SCREEN_WIDTH / 2.0f) - (btnExitNormal.width / 2.0f), (SCREEN_HEIGHT / 2.0f) + 80.0f, (float)btnExitNormal.width, (float)btnExitNormal.height};

    btnMenuExitNormal = LoadTexture("assets/BTN_EXIT_1.png");
    btnMenuExitHover = LoadTexture("assets/BTN_EXIT_PRESSED_2.png");

    btnSettingsBounds = (Rectangle){(SCREEN_WIDTH / 2.0f) - 141.0f, 395.0f, 281.0f, 104.0f}; 
    // Thu nhỏ hitbox của nút Start vào chính giữa để không bị bấm nhầm ở rìa ngoài trong suốt
    btnStartBounds = (Rectangle){(SCREEN_WIDTH / 2.0f) - 100.0f, 220.0f, 200.0f, 130.0f};

    LoadScores();
    InitGame();

    RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose() && !quitGame) {
        UpdateGame();
        
        // Vẽ toàn bộ game lên texture ảo 800x600
        BeginTextureMode(target);
        DrawGame();
        EndTextureMode();
        
        // Vẽ texture ra màn hình thực tế có tự động scale và canh giữa
        BeginDrawing();
        ClearBackground(BLACK);
        
        float scale = fminf((float)GetScreenWidth() / SCREEN_WIDTH, (float)GetScreenHeight() / SCREEN_HEIGHT);
        Rectangle source = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
        Rectangle dest = {
            (GetScreenWidth() - (SCREEN_WIDTH * scale)) * 0.5f,
            (GetScreenHeight() - (SCREEN_HEIGHT * scale)) * 0.5f,
            SCREEN_WIDTH * scale,
            SCREEN_HEIGHT * scale
        };
        
        DrawTexturePro(target.texture, source, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);

    UnloadTexture(btnStartNormal);
    UnloadTexture(btnStartHover);
    UnloadTexture(btnSettingsNormal);
    UnloadTexture(btnSettingsHover);
    UnloadTexture(btnExitNormal);
    UnloadTexture(btnExitHover);
    UnloadTexture(arrowLeftNormal);
    UnloadTexture(arrowLeftHover);
    UnloadTexture(arrowRightNormal);
    UnloadTexture(arrowRightHover);
    UnloadTexture(btnBackNormal);
    UnloadTexture(btnSettingsHover);
    UnloadTexture(btnMenuExitNormal);
    UnloadTexture(btnMenuExitHover);
    UnloadTexture(texHead);
    UnloadTexture(texBody);
    UnloadTexture(texFood);
    UnloadTexture(texPinkOrb);
    UnloadTexture(texGoldOrb);       
    UnloadTexture(texGreenOrb);
    UnloadTexture(texTrophy);
    UnloadTexture(texBomb);
    UnloadTexture(texSoundOn);
    UnloadTexture(texSoundOff);
    UnloadTexture(texBorder);

    UnloadMusicStream(bgmMenu);
    UnloadMusicStream(bgmEasy);
    UnloadMusicStream(bgmNormal);
    UnloadMusicStream(bgmHard);
    UnloadMusicStream(bgmLose);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}

Vector2 GetValidSpawnPosition(void) {
    Vector2 pos;
    bool valid = false;
    while (!valid) {
        pos.x = (float)GetRandomValue(0, GRID_WIDTH - 1);
        pos.y = (float)GetRandomValue(0, GRID_HEIGHT - 1);
        valid = true;
        for (int i = 0; i < snake.length; i++) {
            if (pos.x == snake.body[i].x && pos.y == snake.body[i].y) { valid = false; break; }
        }
        if (!valid) continue;
        if (food.active && pos.x == food.position.x && pos.y == food.position.y) valid = false;
        for(int i=0; i<20; i++) if (frenzyFoods[i].active && pos.x == frenzyFoods[i].position.x && pos.y == frenzyFoods[i].position.y) valid = false;
        if (greenOrb.active && pos.x == greenOrb.position.x && pos.y == greenOrb.position.y) valid = false;
        if (goldOrb.active && pos.x == goldOrb.position.x && pos.y == goldOrb.position.y) valid = false;
        if (pinkOrb.active && pos.x == pinkOrb.position.x && pos.y == pinkOrb.position.y) valid = false;
        for (int i = 0; i < numBombs; i++) {
            if (bombs[i].active && pos.x == bombs[i].position.x && pos.y == bombs[i].position.y) { valid = false; break; }
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
    if (r > 70 && r <= 90) count = 3;
    if (r > 90) count = 4;

    for (int c = 0; c < count; c++) {
        if (numBombs >= MAX_BOMBS) return;
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
    numBombs = 0; // Clear all bombs
    for(int i=0; i<20; i++) {
        frenzyFoods[i].position = GetValidSpawnPosition();
        frenzyFoods[i].active = true;
        frenzyFoods[i].color = (Color){255, 50, 80, 255};
    }
}

void InitGame(void) {
    snake.length = 4;
    snake.body[0] = (Vector2){(float)GRID_WIDTH / 2.0f, (float)GRID_HEIGHT / 2.0f};
    snake.body[1] = (Vector2){(float)GRID_WIDTH / 2.0f - 1.0f, (float)GRID_HEIGHT / 2.0f};
    snake.body[2] = (Vector2){(float)GRID_WIDTH / 2.0f - 2.0f, (float)GRID_HEIGHT / 2.0f};
    snake.body[3] = (Vector2){(float)GRID_WIDTH / 2.0f - 3.0f, (float)GRID_HEIGHT / 2.0f};
    snake.direction = (Vector2){1, 0};
    snake.inputCount = 0;

    score = 0;
    framesCounter = 0;

    if (difficulty == 0) baseGameSpeed = 16;
    else if (difficulty == 1) baseGameSpeed = 12;
    else if (difficulty == 2) baseGameSpeed = 8;

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
    for(int i=0; i<20; i++) frenzyFoods[i].active = false;
    SpawnFood();
}

void UpdateGame(void) 
{
    if (IsKeyPressed(KEY_F11)) {
        if (!IsWindowState(FLAG_WINDOW_UNDECORATED)) {
            SetWindowState(FLAG_WINDOW_UNDECORATED);
            MaximizeWindow();
        } else {
            RestoreWindow();
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
            SetWindowPosition((GetMonitorWidth(GetCurrentMonitor()) - SCREEN_WIDTH) / 2, (GetMonitorHeight(GetCurrentMonitor()) - SCREEN_HEIGHT) / 2);
        }
    }

    Music *targetMusic = &bgmMenu;
    if (currentScreen == PLAYING || currentScreen == PAUSE) {
        if (difficulty == 0) targetMusic = &bgmEasy;
        else if (difficulty == 1) targetMusic = &bgmNormal;
        else if (difficulty == 2) targetMusic = &bgmHard;
    } else if (currentScreen == GAME_OVER) {
        targetMusic = &bgmLose;
    }
    SwitchMusic(targetMusic);

    if (currentMusic != NULL) {
        UpdateMusicStream(*currentMusic);
    }

    float dt = GetFrameTime();
    timeAlive += dt;
    
    if (currentScreen == MENU || currentScreen == SETTINGS) {
        UpdateFallingIcons(dt);
    }
    
    if (currentScreen == PLAYING) {
        if (greenOrb.active) { greenOrb.timer -= dt; if (greenOrb.timer <= 0) greenOrb.active = false; }
        if (goldOrb.active) { goldOrb.timer -= dt; if (goldOrb.timer <= 0) goldOrb.active = false; }
        if (pinkOrb.active) { pinkOrb.timer -= dt; if (pinkOrb.timer <= 0) pinkOrb.active = false; }
    }

    switch (currentScreen) {
    case MENU: 
    {
        Vector2 mousePoint = GetVirtualMousePosition();
        isStartHovered = CheckCollisionPointRec(mousePoint, btnStartBounds);
        isVolMinusHovered = CheckCollisionPointRec(mousePoint, volMinusBounds);
        isVolPlusHovered = CheckCollisionPointRec(mousePoint, volPlusBounds);
        isHighScoresHovered = CheckCollisionPointRec(mousePoint, btnHighScoresBounds);
        isMenuExitHovered = CheckCollisionPointRec(mousePoint, btnMenuExitBounds);

        // Xử lý thoát game
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isMenuExitHovered) {
            quitGame = true;
        }

        // Xử lý hiển thị điểm cao khi di chuột
        showHighScores = isHighScoresHovered;

        // Xử lý chỉnh âm lượng
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (isVolMinusHovered) {
                globalVolume -= 0.1f;
                if (globalVolume < 0.0f) globalVolume = 0.0f;
                isMuted = false;
                SetMasterVolume(globalVolume);
            }
            if (isVolPlusHovered) {
                globalVolume += 0.1f;
                if (globalVolume > 1.0f) globalVolume = 1.0f;
                isMuted = false;
                SetMasterVolume(globalVolume);
            }
        }

        if (isStartHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            InitGame();
            currentScreen = PLAYING;
        }
        
        // === KIỂM TRA CLICK NÚT SETTINGS ===
        isSettingsHovered = CheckCollisionPointRec(mousePoint, btnSettingsBounds);

        if (isSettingsHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            currentScreen = SETTINGS;
        }

        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            menuSelection++;
            if (menuSelection > 1) menuSelection = 0;
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            menuSelection--;
            if (menuSelection < 0) menuSelection = 1;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (menuSelection == 0) {
                InitGame();
                currentScreen = PLAYING;
            } else if (menuSelection == 1) {
                currentScreen = SETTINGS;
            }
        }
    } break;

    case SETTINGS: 
    {
        Vector2 mousePoint = GetVirtualMousePosition();
        
        // Tạo một điểm neo cố định (bạn có thể thay đổi số 100.0f để dịch toàn bộ cụm này sang trái/phải cho ưng mắt)
        float valueCenterX = SCREEN_WIDTH / 2.0f + 120.0f; 

        const char *diffStr = (difficulty == 0) ? "EASY" : (difficulty == 2) ? "HARD" : "NORMAL";
        int wordWidth = MeasureText(diffStr, 30);
        
        // Đặt tâm của chữ EASY/NORMAL/HARD dính chặt vào điểm neo cố định
        float wordX = valueCenterX - (wordWidth / 2.0f);
        
        // Cập nhật lại tọa độ X cho 2 khung nút bấm bám theo 2 bên chữ
        arrowLeftBounds.x = wordX - 48 - 5; 
        arrowRightBounds.x = wordX + wordWidth + 5;

        // Cập nhật tọa độ cho nút Grid
        const char *gridStr = showGrid ? "ON" : "OFF";
        int gridWordWidth = MeasureText(gridStr, 30);
        float gridWordX = valueCenterX - (gridWordWidth / 2.0f);
        gridArrowLeftBounds.x = gridWordX - 48 - 5;
        gridArrowRightBounds.x = gridWordX + gridWordWidth + 5;

        // Cập nhật tọa độ cho nút Moveset
        const char *movesetStr = (moveset == 0) ? "ARROWS" : "WASD";
        int movesetWordWidth = MeasureText(movesetStr, 30);
        float movesetWordX = valueCenterX - (movesetWordWidth / 2.0f);
        movesetArrowLeftBounds.x = movesetWordX - 48 - 5;
        movesetArrowRightBounds.x = movesetWordX + movesetWordWidth + 5;

        // Cập nhật tọa độ cho nút Skin
        const char *skinStr = (snakeSkin == 0) ? "DEFAULT" : (snakeSkin == 1) ? "GREEN" : (snakeSkin == 2) ? "BLUE" : (snakeSkin == 3) ? "RED" : "GOLD";
        int skinWordWidth = MeasureText(skinStr, 30);
        float skinWordX = valueCenterX - (skinWordWidth / 2.0f);
        skinArrowLeftBounds.x = skinWordX - 48 - 5;
        skinArrowRightBounds.x = skinWordX + skinWordWidth + 5;

        // === TẠO HITBOX "TÀNG HÌNH" TO HƠN CHO NÚT BACK ===
        float backPadding = 15.0f;
        float offsetX = 0.0f;  
        float offsetY = 50.0f; 

        Rectangle backHitbox = {
            btnBackBounds.x - backPadding + offsetX,
            btnBackBounds.y - backPadding + offsetY,
            btnBackBounds.width + (backPadding * 2),
            btnBackBounds.height + (backPadding * 2)
        };
        
        isLeftHovered = CheckCollisionPointRec(mousePoint, arrowLeftBounds);
        isRightHovered = CheckCollisionPointRec(mousePoint, arrowRightBounds);
        isGridLeftHovered = CheckCollisionPointRec(mousePoint, gridArrowLeftBounds);
        isGridRightHovered = CheckCollisionPointRec(mousePoint, gridArrowRightBounds);
        isMovesetLeftHovered = CheckCollisionPointRec(mousePoint, movesetArrowLeftBounds);
        isMovesetRightHovered = CheckCollisionPointRec(mousePoint, movesetArrowRightBounds);
        isSkinLeftHovered = CheckCollisionPointRec(mousePoint, skinArrowLeftBounds);
        isSkinRightHovered = CheckCollisionPointRec(mousePoint, skinArrowRightBounds);
        isBackHovered = CheckCollisionPointRec(mousePoint, backHitbox);
        isMuteHovered = CheckCollisionPointRec(mousePoint, btnMuteBounds);

        // Xử lý Click
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (isMuteHovered) {
                isMuted = !isMuted;
                if (isMuted) {
                    SetMasterVolume(0.0f);
                } else {
                    SetMasterVolume(globalVolume);
                }
            }
            if (isLeftHovered) {
                difficulty--;
                if (difficulty < 0) difficulty = 2;
            }
            if (isRightHovered) {
                difficulty++;
                if (difficulty > 2) difficulty = 0;
            }
            if (isGridLeftHovered || isGridRightHovered) {
                showGrid = !showGrid;
            }
            if (isMovesetLeftHovered || isMovesetRightHovered) {
                moveset = (moveset == 0) ? 1 : 0;
            }
            if (isSkinLeftHovered) {
                snakeSkin--;
                if (snakeSkin < 0) snakeSkin = 4;
            }
            if (isSkinRightHovered) {
                snakeSkin++;
                if (snakeSkin > 4) snakeSkin = 0;
            }
            if (isBackHovered) {
                currentScreen = MENU;
            }
        }

        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            settingsSelection++;
            if (settingsSelection > 4) settingsSelection = 0;
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            settingsSelection--;
            if (settingsSelection < 0) settingsSelection = 4;
        }

        if (settingsSelection == 0) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                difficulty--;
                if (difficulty < 0) difficulty = 2;
            }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                difficulty++;
                if (difficulty > 2) difficulty = 0;
            }
        }
        else if (settingsSelection == 1) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                showGrid = !showGrid;
            }
        }
        else if (settingsSelection == 2) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                moveset = (moveset == 0) ? 1 : 0;
            }
        }
        else if (settingsSelection == 3) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                snakeSkin--;
                if (snakeSkin < 0) snakeSkin = 4;
            }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                snakeSkin++;
                if (snakeSkin > 4) snakeSkin = 0;
            }
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (settingsSelection == 4) {
                currentScreen = MENU;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
            currentScreen = MENU;
        }
    } break;

    case PLAYING:
    {
        bool upPressed = (moveset == 0) ? IsKeyPressed(KEY_UP) : IsKeyPressed(KEY_W);
        bool downPressed = (moveset == 0) ? IsKeyPressed(KEY_DOWN) : IsKeyPressed(KEY_S);
        bool leftPressed = (moveset == 0) ? IsKeyPressed(KEY_LEFT) : IsKeyPressed(KEY_A);
        bool rightPressed = (moveset == 0) ? IsKeyPressed(KEY_RIGHT) : IsKeyPressed(KEY_D);

        Vector2 lastDir = (snake.inputCount > 0) ? snake.inputQueue[snake.inputCount - 1] : snake.direction;

        if (upPressed && lastDir.y != 1 && snake.inputCount < MAX_INPUT_QUEUE) {
            snake.inputQueue[snake.inputCount++] = (Vector2){0, -1};
            lastDir = (Vector2){0, -1};
        } 
        if (downPressed && lastDir.y != -1 && snake.inputCount < MAX_INPUT_QUEUE) {
            snake.inputQueue[snake.inputCount++] = (Vector2){0, 1};
            lastDir = (Vector2){0, 1};
        } 
        if (leftPressed && lastDir.x != 1 && snake.inputCount < MAX_INPUT_QUEUE) {
            snake.inputQueue[snake.inputCount++] = (Vector2){-1, 0};
            lastDir = (Vector2){-1, 0};
        } 
        if (rightPressed && lastDir.x != -1 && snake.inputCount < MAX_INPUT_QUEUE) {
            snake.inputQueue[snake.inputCount++] = (Vector2){1, 0};
            lastDir = (Vector2){1, 0};
        }

        framesCounter++;
        if (framesCounter >= gameSpeed) {
            framesCounter = 0;
            
            if (snake.inputCount > 0) {
                snake.direction = snake.inputQueue[0];
                for (int i = 0; i < snake.inputCount - 1; i++) {
                    snake.inputQueue[i] = snake.inputQueue[i + 1];
                }
                snake.inputCount--;
            }

            for (int i = snake.length - 1; i > 0; i--) {
                snake.body[i] = snake.body[i - 1];
            }

            snake.body[0].x += snake.direction.x;
            snake.body[0].y += snake.direction.y;

            // Wall Collision
            if (snake.body[0].x < 0 || snake.body[0].x >= GRID_WIDTH ||
                snake.body[0].y < 0 || snake.body[0].y >= GRID_HEIGHT) {
                TriggerGameOver();
            }

            // Self Collision
            for (int i = 1; i < snake.length; i++) {
                if (snake.body[0].x == snake.body[i].x && snake.body[0].y == snake.body[i].y) {
                    TriggerGameOver();
                }
            }

            // Green Orb Collision
            if (greenOrb.active && snake.body[0].x == greenOrb.position.x && snake.body[0].y == greenOrb.position.y) {
                greenOrb.active = false;
                int newLength = (int)(snake.length * 0.7f);
                if (newLength < 4) newLength = 4;
                snake.length = newLength;
            }
            
            // Gold Orb Collision
            if (goldOrb.active && snake.body[0].x == goldOrb.position.x && snake.body[0].y == goldOrb.position.y) {
                goldOrb.active = false;
                score += 50;
            }
            
            // Pink Orb Collision
            if (pinkOrb.active && snake.body[0].x == pinkOrb.position.x && snake.body[0].y == pinkOrb.position.y) {
                pinkOrb.active = false;
                TriggerFrenzy();
            }

            // Bomb Collision
            for (int i = 0; i < numBombs; i++) {
                if (bombs[i].active && snake.body[0].x == bombs[i].position.x && snake.body[0].y == bombs[i].position.y) {
                    TriggerGameOver();
                }
            }

            bool ateFood = false;

            // Regular Food Collision
            if (food.active && snake.body[0].x == food.position.x && snake.body[0].y == food.position.y) {
                ateFood = true;
                SpawnFood();
            }

            // Frenzy Food Collision
            if (frenzyActive) {
                for(int i=0; i<20; i++) {
                    if (frenzyFoods[i].active && snake.body[0].x == frenzyFoods[i].position.x && snake.body[0].y == frenzyFoods[i].position.y) {
                        ateFood = true;
                        frenzyFoods[i].position = GetValidSpawnPosition();
                    }
                }
            }

            if (ateFood) {
                if (snake.length < MAX_SNAKE_LENGTH) {
                    snake.body[snake.length] = snake.body[snake.length - 1]; // Cập nhật toạ độ cho đốt mới ngay lập tức
                    snake.length++;
                }
                score += 10;
                normalFoodsEaten++;

                if (!goldOrb.active && GetRandomValue(1, 100) <= 10) SpawnGoldOrb();
                if (!pinkOrb.active && GetRandomValue(1, 100) <= 12) SpawnPinkOrb();

                if (!frenzyActive) {
                    foodsSinceBombClear++;
                    
                    int bombSpawnFreq = 4; // default
                    if (difficulty == 0) bombSpawnFreq = 5;
                    else if (difficulty == 1) bombSpawnFreq = 4;
                    else if (difficulty == 2) bombSpawnFreq = 3;

                    if (score > 0 && score % (bombSpawnFreq * 10) == 0) SpawnBomb();

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

                if (score % 50 == 0 && gameSpeed > 4) gameSpeed--;
            }

            if (frenzyActive) {
                frenzyTimer -= (float)gameSpeed / 60.0f;
                if (frenzyTimer <= 0.0f) {
                    frenzyActive = false;
                    for(int i=0; i<20; i++) frenzyFoods[i].active = false;
                }
            }
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
            currentScreen = PAUSE;
            pauseSelection = 0;
        }
    } break;

    case PAUSE:
    {
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            pauseSelection++;
            if (pauseSelection > 2) pauseSelection = 0;
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            pauseSelection--;
            if (pauseSelection < 0) pauseSelection = 2;
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
    } break;

    case GAME_OVER:
    {
        Vector2 mousePoint = GetVirtualMousePosition();
        isExitHovered = CheckCollisionPointRec(mousePoint, btnExitBounds);

        if (isExitHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            currentScreen = MENU; // Quay lại menu game
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            currentScreen = MENU;
        }
    } break;
    }
}

void DrawGame(void) 
{
    Color bgDark = (Color){20, 24, 34, 255};
    Color bgMenu = (Color){15, 18, 25, 255};
    Color gridColor = (Color){31, 38, 51, 230};

    ClearBackground(bgDark);

    if (currentScreen == MENU || currentScreen == SETTINGS) {
        DrawFallingIcons();
    }

    switch (currentScreen) {
    case MENU: 
    {
        int titleWidth = MeasureText("Snake", 80);
        Color titleColor = (Color){60, 240, 170, 255};
        DrawText("Snake", SCREEN_WIDTH / 2 - titleWidth / 2, 100, 80, titleColor);

        // Hardcode tọa độ in hình gốc để ảnh không bị lệch khi hitbox đã co lại
        int startDrawX = SCREEN_WIDTH / 2 - 155;
        int startDrawY = 180;
        
        if (isStartHovered) {
            DrawTexture(btnStartHover, startDrawX, startDrawY, WHITE);
        } else {
            DrawTexture(btnStartNormal, startDrawX, startDrawY, WHITE);
        }
        if (isSettingsHovered) {
            DrawTexture(btnSettingsHover, btnSettingsBounds.x, btnSettingsBounds.y, WHITE);
        } else {
            DrawTexture(btnSettingsNormal, btnSettingsBounds.x, btnSettingsBounds.y, WHITE);
        }

        // --- VẼ NÚT TOP SCORES (LÀ HÌNH CHIẾC CÚP) ---
        Rectangle trophySource = {0.0f, 0.0f, (float)texTrophy.width, (float)texTrophy.height};
        Rectangle trophyDest = {btnHighScoresBounds.x, btnHighScoresBounds.y, btnHighScoresBounds.width, btnHighScoresBounds.height};
        DrawTexturePro(texTrophy, trophySource, trophyDest, (Vector2){0, 0}, 0.0f, isHighScoresHovered ? WHITE : LIGHTGRAY);

        // --- NẾU BẬT THÌ HIỂN THỊ BẢNG TOP SCORES ---
        if (showHighScores) {
            Rectangle panelBounds = {btnHighScoresBounds.x, btnHighScoresBounds.y - 110, 180, 100};
            DrawRectangleRec(panelBounds, Fade(BLACK, 0.8f));
            DrawRectangleLinesEx(panelBounds, 2.0f, (Color){255, 215, 0, 255});
            DrawText("TOP 3 SCORES", panelBounds.x + 15, panelBounds.y + 10, 20, (Color){255, 215, 0, 255});
            for (int i = 0; i < 3; i++) {
                DrawText(TextFormat("%d. %04d", i + 1, topScores[i]), panelBounds.x + 15, panelBounds.y + 40 + (i * 20), 20, LIGHTGRAY);
            }
        }

        // --- VẼ CỤM VOLUME ---
        DrawRectangleRec(volMinusBounds, isVolMinusHovered ? LIGHTGRAY : GRAY);
        DrawRectangleRec(volPlusBounds, isVolPlusHovered ? LIGHTGRAY : GRAY);
        DrawText("-", volMinusBounds.x + 10, volMinusBounds.y + 5, 20, BLACK);
        DrawText("+", volPlusBounds.x + 7, volPlusBounds.y + 5, 20, BLACK);
        
        int volPercent = isMuted ? 0 : (int)(globalVolume * 100.0f + 0.5f);
        const char *volText = TextFormat("VOLUME: %d%%", volPercent);
        int textWidth = MeasureText(volText, 20);
        int textX = volMinusBounds.x + 30 + ((volPlusBounds.x - volMinusBounds.x - 30) - textWidth) / 2;
        DrawText(volText, textX, volMinusBounds.y + 5, 20, WHITE);

        // --- VẼ NÚT EXIT MENU ---
        Rectangle exitSource = {0.0f, 0.0f, (float)btnMenuExitNormal.width, (float)btnMenuExitNormal.height};
        Rectangle exitDest = {btnMenuExitBounds.x, btnMenuExitBounds.y, btnMenuExitBounds.width, btnMenuExitBounds.height};
        DrawTexturePro(isMenuExitHovered ? btnMenuExitHover : btnMenuExitNormal, exitSource, exitDest, (Vector2){0, 0}, 0.0f, WHITE);
    } break; 
        
    case SETTINGS: 
    {
        int titleWidth = MeasureText("SETTINGS", 60);
        DrawText("SETTINGS", SCREEN_WIDTH / 2 - titleWidth / 2, 100, 60, LIGHTGRAY);

        DrawTextureEx(isLeftHovered ? arrowLeftHover : arrowLeftNormal, (Vector2){arrowLeftBounds.x, arrowLeftBounds.y}, 0.0f, 3.0f, WHITE);
        DrawTextureEx(isRightHovered ? arrowRightHover : arrowRightNormal, (Vector2){arrowRightBounds.x, arrowRightBounds.y}, 0.0f, 3.0f, WHITE);
        
        DrawTextureEx(isGridLeftHovered ? arrowLeftHover : arrowLeftNormal, (Vector2){gridArrowLeftBounds.x, gridArrowLeftBounds.y}, 0.0f, 3.0f, WHITE);
        DrawTextureEx(isGridRightHovered ? arrowRightHover : arrowRightNormal, (Vector2){gridArrowRightBounds.x, gridArrowRightBounds.y}, 0.0f, 3.0f, WHITE);

        DrawTextureEx(isMovesetLeftHovered ? arrowLeftHover : arrowLeftNormal, (Vector2){movesetArrowLeftBounds.x, movesetArrowLeftBounds.y}, 0.0f, 3.0f, WHITE);
        DrawTextureEx(isMovesetRightHovered ? arrowRightHover : arrowRightNormal, (Vector2){movesetArrowRightBounds.x, movesetArrowRightBounds.y}, 0.0f, 3.0f, WHITE);

        DrawTextureEx(isSkinLeftHovered ? arrowLeftHover : arrowLeftNormal, (Vector2){skinArrowLeftBounds.x, skinArrowLeftBounds.y}, 0.0f, 3.0f, WHITE);
        DrawTextureEx(isSkinRightHovered ? arrowRightHover : arrowRightNormal, (Vector2){skinArrowRightBounds.x, skinArrowRightBounds.y}, 0.0f, 3.0f, WHITE);

        DrawTexture(isBackHovered ? btnBackHover : btnBackNormal, btnBackBounds.x, btnBackBounds.y, WHITE);

        int c1 = settingsSelection == 0 ? 255 : 150;
        int cGrid = settingsSelection == 1 ? 255 : 150;
        int cMove = settingsSelection == 2 ? 255 : 150;
        int cSkin = settingsSelection == 3 ? 255 : 150;
        const char *diffStr = (difficulty == 0) ? "EASY" : (difficulty == 2) ? "HARD" : "NORMAL";
        const char *gridStr = showGrid ? "ON" : "OFF";
        const char *movesetStr = (moveset == 0) ? "ARROWS" : "WASD";
        const char *skinStr = (snakeSkin == 0) ? "DEFAULT" : (snakeSkin == 1) ? "GREEN" : (snakeSkin == 2) ? "BLUE" : (snakeSkin == 3) ? "RED" : "GOLD";
        
        // Neo tọa độ giống hệt bên UpdateGame
        float diffLabelX = SCREEN_WIDTH / 2.0f - 240; // Chữ DIFFICULTY đứng yên bên trái
        float valueCenterX = SCREEN_WIDTH / 2.0f + 120.0f; // Chữ EASY/NORMAL đứng yên bên phải

        int wordWidth = MeasureText(diffStr, 30);
        float wordX = valueCenterX - (wordWidth / 2.0f); 

        int gridWordWidth = MeasureText(gridStr, 30);
        float gridWordX = valueCenterX - (gridWordWidth / 2.0f); 

        int movesetWordWidth = MeasureText(movesetStr, 30);
        float movesetWordX = valueCenterX - (movesetWordWidth / 2.0f); 

        int skinWordWidth = MeasureText(skinStr, 30);
        float skinWordX = valueCenterX - (skinWordWidth / 2.0f);

        // Vẽ riêng chữ "DIFFICULTY:" cố định
        DrawText("DIFFICULTY:", diffLabelX, 205, 30, (Color){c1, c1, c1, 255});
        DrawText(diffStr, wordX, 205, 30, (Color){c1, c1, c1, 255});
        
        DrawText("SHOW GRID:", diffLabelX, 260, 30, (Color){cGrid, cGrid, cGrid, 255});
        DrawText(gridStr, gridWordX, 260, 30, (Color){cGrid, cGrid, cGrid, 255});

        DrawText("MOVESET:", diffLabelX, 315, 30, (Color){cMove, cMove, cMove, 255});
        DrawText(movesetStr, movesetWordX, 315, 30, (Color){cMove, cMove, cMove, 255});

        DrawText("SKIN:", diffLabelX, 370, 30, (Color){cSkin, cSkin, cSkin, 255});
        DrawText(skinStr, skinWordX, 370, 30, (Color){cSkin, cSkin, cSkin, 255});
        
        // Vẽ nút tắt/mở âm thanh
        DrawRectangleRec(btnMuteBounds, isMuteHovered ? SKYBLUE : BLUE);
        Texture2D currentSoundIcon = isMuted ? texSoundOff : texSoundOn;
        Rectangle soundSource = { 0.0f, 0.0f, (float)currentSoundIcon.width, (float)currentSoundIcon.height };
        Rectangle soundDest = { btnMuteBounds.x, btnMuteBounds.y, btnMuteBounds.width, btnMuteBounds.height };
        DrawTexturePro(currentSoundIcon, soundSource, soundDest, (Vector2){0,0}, 0.0f, WHITE);
        
    } break;
    
    case PAUSE:
    case PLAYING: 
    {
        DrawRectangle(0, 0, SCREEN_WIDTH, MENU_HEIGHT, bgMenu);
        DrawLine(0, MENU_HEIGHT, SCREEN_WIDTH, MENU_HEIGHT, (Color){50, 60, 80, 255});
        DrawTextEx(GetFontDefault(), TextFormat("SCORE: %04d", score), (Vector2){20, 15}, 22, 2, (Color){255, 215, 0, 255}); // Đổi thành màu vàng nổi bật
        DrawTextEx(GetFontDefault(), TextFormat("BEST: %04d", topScores[0]), (Vector2){SCREEN_WIDTH - 150, 15}, 22, 2, (Color){255, 215, 0, 255});

        Vector2 offset = {BORDER_MARGIN, MENU_HEIGHT + BORDER_MARGIN};

        // Vẽ lớp Grid và các hạt sao lấp lánh
        if (showGrid) {
            for (int i = 0; i < MAX_STARS; i++) {
                float twinkle = (sinf(timeAlive * 3.0f + backgroundStars[i].phase) + 1.0f) / 2.0f;
                Color starColor = backgroundStars[i].color;
                starColor.a = (unsigned char)(starColor.a * twinkle);
                DrawRectangle(
                    offset.x + backgroundStars[i].position.x, 
                    offset.y + backgroundStars[i].position.y, 
                    backgroundStars[i].size, backgroundStars[i].size, 
                    starColor
                );
            }
            for (int i = 0; i <= GRID_WIDTH; i++) {
                DrawLineEx((Vector2){offset.x + i * CELL_SIZE, offset.y}, (Vector2){offset.x + i * CELL_SIZE, offset.y + GRID_HEIGHT * CELL_SIZE}, 1.0f, gridColor);
            }
            for (int i = 0; i <= GRID_HEIGHT; i++) {
                DrawLineEx((Vector2){offset.x, offset.y + i * CELL_SIZE}, (Vector2){offset.x + GRID_WIDTH * CELL_SIZE, offset.y + i * CELL_SIZE}, 1.0f, gridColor);
            }
        }


        // --- VẼ THỨC ĂN CHÍNH (Có hiệu ứng to nhỏ) ---
        float pulse = (sinf(timeAlive * 6.0f) + 1.0f) / 2.0f;
        float foodDisplaySize = CELL_SIZE * 0.8f + pulse * (CELL_SIZE * 0.2f); // Kích thước dao động

        if (food.active) {
            Rectangle source = { 0.0f, 0.0f, (float)texFood.width, (float)texFood.height };
            Rectangle dest = { 
                food.position.x * CELL_SIZE + offset.x + CELL_SIZE / 2.0f, 
                food.position.y * CELL_SIZE + offset.y + CELL_SIZE / 2.0f, 
                foodDisplaySize, foodDisplaySize 
            };
            Vector2 origin = { foodDisplaySize / 2.0f, foodDisplaySize / 2.0f };
            Rectangle shadowDest = { dest.x + 4.0f, dest.y + 4.0f, dest.width, dest.height };
            DrawTexturePro(texFood, source, shadowDest, origin, 0.0f, (Color){0, 0, 0, 100});
            DrawTexturePro(texFood, source, dest, origin, 0.0f, WHITE);
        }

        // --- VẼ THỨC ĂN FRENZY (Chế độ ăn lẹ) ---
        if (frenzyActive) {
            Rectangle source = { 0.0f, 0.0f, (float)texFood.width, (float)texFood.height };
            for(int i=0; i<20; i++) {
                if (frenzyFoods[i].active) {
                    Rectangle dest = { 
                        frenzyFoods[i].position.x * CELL_SIZE + offset.x + CELL_SIZE / 2.0f, 
                        frenzyFoods[i].position.y * CELL_SIZE + offset.y + CELL_SIZE / 2.0f, 
                        foodDisplaySize, foodDisplaySize 
                    };
                    Vector2 origin = { foodDisplaySize / 2.0f, foodDisplaySize / 2.0f };
                    Rectangle shadowDest = { dest.x + 4.0f, dest.y + 4.0f, dest.width, dest.height };
                    DrawTexturePro(texFood, source, shadowDest, origin, 0.0f, (Color){0, 0, 0, 100});
                    DrawTexturePro(texFood, source, dest, origin, 0.0f, WHITE);
                }
            }
        }
        // --- VẼ QUẢ BONUS MÀU XANH (Green Orb) BẰNG ẢNH ---
        if (greenOrb.active) {
            // Giữ nguyên nhịp đập của quả xanh cũ (10.0f)
            float orbPulse = (sinf(timeAlive * 10.0f) + 1.0f) / 2.0f;
            float orbSize = CELL_SIZE * 0.8f + orbPulse * (CELL_SIZE * 0.15f);

            Rectangle source = { 0.0f, 0.0f, (float)texGreenOrb.width, (float)texGreenOrb.height };
            Rectangle dest = {
                greenOrb.position.x * CELL_SIZE + offset.x + CELL_SIZE / 2.0f,
                greenOrb.position.y * CELL_SIZE + offset.y + CELL_SIZE / 2.0f,
                orbSize, orbSize
            };
            Vector2 origin = { orbSize / 2.0f, orbSize / 2.0f };
            Rectangle shadowDest = { dest.x + 4.0f, dest.y + 4.0f, dest.width, dest.height };
            DrawTexturePro(texGreenOrb, source, shadowDest, origin, 0.0f, (Color){0, 0, 0, 100});
            DrawTexturePro(texGreenOrb, source, dest, origin, 0.0f, WHITE);
        }

        // --- VẼ QUẢ BONUS MÀU VÀNG BẰNG ẢNH ---
        if (goldOrb.active) {
            // Giữ nguyên nhịp đập nhanh hơn quả hồng một chút (12.0f)
            float orbPulse = (sinf(timeAlive * 12.0f) + 1.0f) / 2.0f;
            float orbSize = CELL_SIZE * 0.8f + orbPulse * (CELL_SIZE * 0.2f);
            
            Rectangle source = { 0.0f, 0.0f, (float)texGoldOrb.width, (float)texGoldOrb.height };
            Rectangle dest = { 
                goldOrb.position.x * CELL_SIZE + offset.x + CELL_SIZE / 2.0f, 
                goldOrb.position.y * CELL_SIZE + offset.y + CELL_SIZE / 2.0f, 
                orbSize, orbSize 
            };
            Vector2 origin = { orbSize / 2.0f, orbSize / 2.0f };
            Rectangle shadowDest = { dest.x + 4.0f, dest.y + 4.0f, dest.width, dest.height };
            DrawTexturePro(texGoldOrb, source, shadowDest, origin, 0.0f, (Color){0, 0, 0, 100});
            DrawTexturePro(texGoldOrb, source, dest, origin, 0.0f, WHITE);
        }
        

        // --- VẼ QUẢ BONUS MÀU HỒNG ---
        if (pinkOrb.active) {
            float orbPulse = (sinf(timeAlive * 8.0f) + 1.0f) / 2.0f;
            float orbSize = CELL_SIZE * 0.8f + orbPulse * (CELL_SIZE * 0.2f);
            
            Rectangle source = { 0.0f, 0.0f, (float)texPinkOrb.width, (float)texPinkOrb.height };
            Rectangle dest = { 
                pinkOrb.position.x * CELL_SIZE + offset.x + CELL_SIZE / 2.0f, 
                pinkOrb.position.y * CELL_SIZE + offset.y + CELL_SIZE / 2.0f, 
                orbSize, orbSize 
            };
            Vector2 origin = { orbSize / 2.0f, orbSize / 2.0f };
            Rectangle shadowDest = { dest.x + 4.0f, dest.y + 4.0f, dest.width, dest.height };
            DrawTexturePro(texPinkOrb, source, shadowDest, origin, 0.0f, (Color){0, 0, 0, 100});
            DrawTexturePro(texPinkOrb, source, dest, origin, 0.0f, WHITE);
        }

        for (int i = 0; i < numBombs; i++) {
            if (!bombs[i].active) continue;
            float bombPulse = (sinf(timeAlive * 8.0f) + 1.0f) / 2.0f;
            float bombSize = CELL_SIZE * 0.8f + bombPulse * (CELL_SIZE * 0.3f);
            
            Rectangle source = { 0.0f, 0.0f, (float)texBomb.width, (float)texBomb.height };
            Rectangle dest = { 
                bombs[i].position.x * CELL_SIZE + offset.x + CELL_SIZE / 2.0f, 
                bombs[i].position.y * CELL_SIZE + offset.y + CELL_SIZE / 2.0f, 
                bombSize, bombSize 
            };
            Vector2 origin = { bombSize / 2.0f, bombSize / 2.0f };
            // Tạo hiệu ứng lắc nhẹ qua lại cho bom
            float rotation = sinf(timeAlive * 10.0f) * 15.0f; 
            
            DrawTexturePro(texBomb, source, dest, origin, rotation, WHITE);
            
            // Vẽ viền đỏ nhấp nháy báo hiệu bom xung quanh
            DrawCircleLines(dest.x, dest.y, bombSize * 0.5f + bombPulse * 2.0f, (Color){255, 50, 50, 150});
        }

        // --- XÁC ĐỊNH MÀU SKIN ---
        Color skinColor = WHITE;
        if (snakeSkin == 1) skinColor = GREEN;
        else if (snakeSkin == 2) skinColor = SKYBLUE;
        else if (snakeSkin == 3) skinColor = RED;
        else if (snakeSkin == 4) skinColor = GOLD;

        // --- VẼ THÂN RẮN BẰNG HÌNH ẢNH ---
        for (int i = snake.length - 1; i > 0; i--) {
            // Tính toán hướng của đốt thân này so với đốt phía trước nó
            float dx = snake.body[i - 1].x - snake.body[i].x;
            float dy = snake.body[i - 1].y - snake.body[i].y;
            
            float bodyRot = 0.0f;
            if (dx == 1) bodyRot = 0.0f;         // Đi sang phải
            else if (dx == -1) bodyRot = 180.0f; // Đi sang trái
            else if (dy == 1) bodyRot = 90.0f;   // Đi xuống
            else if (dy == -1) bodyRot = 270.0f; // Đi lên

            Rectangle bodySource = { 0.0f, 0.0f, (float)texBody.width, (float)texBody.height };
            // Đặt tâm điểm (Dest) vào chính giữa ô lưới
            Rectangle bodyDest = { 
                snake.body[i].x * CELL_SIZE + offset.x + CELL_SIZE / 2.0f, 
                snake.body[i].y * CELL_SIZE + offset.y + CELL_SIZE / 2.0f, 
                (float)CELL_SIZE, 
                (float)CELL_SIZE 
            };
            Vector2 origin = { CELL_SIZE / 2.0f, CELL_SIZE / 2.0f };
            Rectangle shadowDest = { bodyDest.x + 4.0f, bodyDest.y + 4.0f, bodyDest.width, bodyDest.height };

            // Vẽ bóng đổ đen mờ
            DrawTexturePro(texBody, bodySource, shadowDest, origin, bodyRot, (Color){0, 0, 0, 100});
            // Màu WHITE nghĩa là giữ nguyên màu gốc của ảnh
            DrawTexturePro(texBody, bodySource, bodyDest, origin, bodyRot, skinColor);
        }

        // --- VẼ ĐẦU RẮN BẰNG HÌNH ẢNH ---
        float headRot = 0.0f;
        if (snake.direction.x == 1) headRot = 0.0f;         // Sang phải
        else if (snake.direction.x == -1) headRot = 180.0f; // Sang trái
        else if (snake.direction.y == 1) headRot = 90.0f;   // Đi xuống
        else if (snake.direction.y == -1) headRot = 270.0f; // Đi lên

        Rectangle headSource = { 0.0f, 0.0f, (float)texHead.width, (float)texHead.height };
        Rectangle headDest = { 
            snake.body[0].x * CELL_SIZE + offset.x + CELL_SIZE / 2.0f, 
            snake.body[0].y * CELL_SIZE + offset.y + CELL_SIZE / 2.0f, 
            (float)CELL_SIZE, 
            (float)CELL_SIZE 
        };
        Vector2 headOrigin = { CELL_SIZE / 2.0f, CELL_SIZE / 2.0f };
        Rectangle shadowDest = { headDest.x + 4.0f, headDest.y + 4.0f, headDest.width, headDest.height };

        DrawTexturePro(texHead, headSource, shadowDest, headOrigin, headRot, (Color){0, 0, 0, 100});
        DrawTexturePro(texHead, headSource, headDest, headOrigin, headRot, skinColor);
            
        // VẼ KHUNG VIỀN PIXEL ART BẰNG CÁCH LÁT GẠCH (TILED BORDER)
        int tileSize = BORDER_MARGIN; // Lát đúng kích thước 25
        Rectangle sourceTile = {0, 0, (float)texBorder.width, (float)texBorder.height};
        
        // Viền trên và dưới (chạy dọc theo chiều ngang)
        for (int x = 0; x < SCREEN_WIDTH; x += tileSize) {
            DrawTexturePro(texBorder, sourceTile, (Rectangle){(float)x, MENU_HEIGHT, (float)tileSize, (float)tileSize}, (Vector2){0,0}, 0.0f, WHITE);
            DrawTexturePro(texBorder, sourceTile, (Rectangle){(float)x, SCREEN_HEIGHT - tileSize, (float)tileSize, (float)tileSize}, (Vector2){0,0}, 0.0f, WHITE);
        }
        
        // Viền trái và phải (bỏ qua mép trên và mép dưới để không bị đè gạch)
        for (int y = MENU_HEIGHT + tileSize; y < SCREEN_HEIGHT - tileSize; y += tileSize) {
            DrawTexturePro(texBorder, sourceTile, (Rectangle){0, (float)y, (float)tileSize, (float)tileSize}, (Vector2){0,0}, 0.0f, WHITE);
            DrawTexturePro(texBorder, sourceTile, (Rectangle){SCREEN_WIDTH - tileSize, (float)y, (float)tileSize, (float)tileSize}, (Vector2){0,0}, 0.0f, WHITE);
        }

        if (currentScreen == PAUSE) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(bgDark, 0.85f));
            DrawText("PAUSED", SCREEN_WIDTH / 2 - MeasureText("PAUSED", 60) / 2, SCREEN_HEIGHT / 2 - 100, 60, (Color){200, 220, 255, 255});
            int c1 = pauseSelection == 0 ? 255 : 150;
            int c2 = pauseSelection == 1 ? 255 : 150;
            int c3 = pauseSelection == 2 ? 255 : 150;
            DrawText("CONTINUE", SCREEN_WIDTH / 2 - MeasureText("CONTINUE", 30) / 2, SCREEN_HEIGHT / 2 - 20, 30, (Color){c1, c1, c1, 255});
            DrawText("MAIN MENU", SCREEN_WIDTH / 2 - MeasureText("MAIN MENU", 30) / 2, SCREEN_HEIGHT / 2 + 30, 30, (Color){c2, c2, c2, 255});
            DrawText("QUIT GAME", SCREEN_WIDTH / 2 - MeasureText("QUIT GAME", 30) / 2, SCREEN_HEIGHT / 2 + 80, 30, (Color){c3, c3, c3, 255});
            float pulsePause = (sinf(timeAlive * 5.0f) + 1.0f) / 2.0f;
            Color highlight = (Color){60, 240, 170, 255};
            if (pauseSelection == 0) DrawText(">", SCREEN_WIDTH / 2 - MeasureText("CONTINUE", 30) / 2 - 30 + pulsePause * 5, SCREEN_HEIGHT / 2 - 20, 30, highlight);
            if (pauseSelection == 1) DrawText(">", SCREEN_WIDTH / 2 - MeasureText("MAIN MENU", 30) / 2 - 30 + pulsePause * 5, SCREEN_HEIGHT / 2 + 30, 30, highlight);
            if (pauseSelection == 2) DrawText(">", SCREEN_WIDTH / 2 - MeasureText("QUIT GAME", 30) / 2 - 30 + pulsePause * 5, SCREEN_HEIGHT / 2 + 80, 30, highlight);
        }
    } break;

    case GAME_OVER: 
    {
        const char *text1 = "GAME OVER";

        const char *flavorText;
        if (score < 500) flavorText = "NOOB! TRY HARDER!";
        else if (score < 1500) flavorText = "NOT BAD, BUT KEEP GOING!";
        else if (score < 3000) flavorText = "GOOD JOB! YOU HAVE POTENTIAL!";
        else if (score < 5000) flavorText = "AWESOME! YOU ARE A SNAKE MASTER!";
        else flavorText = "LEGENDARY! YOU ARE UNSTOPPABLE!";

        int w1 = MeasureText(text1, 60);
        int w3 = MeasureText(flavorText, 25);

        const char *scoreText = TextFormat("SCORE: %d", score);
        int wScore = MeasureText(scoreText, 40);

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(bgDark, 0.85f));

        float pulse = (sinf(timeAlive * 3.0f) + 1.0f) / 2.0f;
        Color warnColor = ColorLerp((Color){230, 60, 60, 255}, (Color){255, 120, 120, 255}, pulse);

        DrawText(text1, SCREEN_WIDTH / 2 - w1 / 2, SCREEN_HEIGHT / 2 - 120, 60, warnColor);
        DrawText(flavorText, SCREEN_WIDTH / 2 - w3 / 2, SCREEN_HEIGHT / 2 - 40, 25, (Color){200, 220, 255, 255});
        DrawText(scoreText, SCREEN_WIDTH / 2 - wScore / 2, SCREEN_HEIGHT / 2 + 10, 40, (Color){255, 215, 0, 255});
        
        // Vẽ nút Exit
        if (isExitHovered) {
            DrawTexture(btnExitHover, btnExitBounds.x, btnExitBounds.y, WHITE);
        } else {
            DrawTexture(btnExitNormal, btnExitBounds.x, btnExitBounds.y, WHITE);
        }
       } break;
    } 
}