/*******************************************************************************************
*
*   raylib - classic game: snake
*
*   Sample game developed by Ian Eito, Albert Martos and Ramon Santamaria
*
*   This game has been created using raylib v1.3 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2015 Ramon Santamaria (@raysan5)
*
********************************************************************************************/



#include <kos.h>
#include <raylib.h>
#include <raymath.h>

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define SNAKE_LENGTH   256
#define SQUARE_SIZE     31

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Snake {
    Vector2 position;
    Vector2 size;
    Vector2 speed;
    Color color;
} Snake;

typedef struct Food {
    Vector2 position;
    Vector2 size;
    bool active;
    Color color;
} Food;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static bool exitGame = false;
static int framesCounter = 0;
static bool gameOver = false;
static bool gamePaused = false;  

static Food fruit = { 0 };
static Snake snake[SNAKE_LENGTH] = { 0 };
static Vector2 snakePosition[SNAKE_LENGTH] = { 0 };
static bool allowMove = false;
static Vector2 offset = { 0 };
static int counterTail = 0;

static maple_device_t *cont;
static cont_state_t *pad_state;

//------------------------------------------------------------------------------------
// Function Declarations
//------------------------------------------------------------------------------------
static void InitGame(void);
static void UpdateGame(void);
static void DrawGame(void);
static void UnloadGame(void);
static void UpdateDrawFrame(void);
static void UpdateController(void);

//------------------------------------------------------------------------------------
// Controller handling
//------------------------------------------------------------------------------------
void UpdateController(void) {
    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        pad_state = (cont_state_t *)maple_dev_status(cont);

        if(!pad_state) {
            printf("Error reading controller\n");
            return;
        }

        // Check for game exit condition
        if(pad_state->buttons & CONT_START) {
            exitGame = true;
        }
    }
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake - Dreamcast");
    InitGame();
    SetTargetFPS(60);

    // Main game loop
    while (!exitGame)
    {
        UpdateController();
        UpdateDrawFrame();
    }

    // De-Initialization
    UnloadGame();
    CloseWindow();
    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions
//------------------------------------------------------------------------------------
void InitGame(void)
{
    framesCounter = 0;
    gameOver = false;
    gamePaused = false;
    counterTail = 1;
    allowMove = false;

    offset.x = SCREEN_WIDTH%SQUARE_SIZE;
    offset.y = SCREEN_HEIGHT%SQUARE_SIZE;

    for (int i = 0; i < SNAKE_LENGTH; i++)
    {
        snake[i].position = (Vector2){ offset.x/2, offset.y/2 };
        snake[i].size = (Vector2){ SQUARE_SIZE, SQUARE_SIZE };
        snake[i].speed = (Vector2){ SQUARE_SIZE, 0 };

        if (i == 0) snake[i].color = DARKBLUE;
        else snake[i].color = BLUE;
    }

    for (int i = 0; i < SNAKE_LENGTH; i++)
    {
        snakePosition[i] = (Vector2){ 0.0f, 0.0f };
    }

    fruit.size = (Vector2){ SQUARE_SIZE, SQUARE_SIZE };
    fruit.color = SKYBLUE;
    fruit.active = false;
}

void UpdateGame(void)
{
    if (!gameOver)
    {
        if (cont && pad_state)
        {
            // Only check for pause if not in game over state
            static bool lastAState = false;
            bool currentAState = (pad_state->buttons & CONT_A);
            
            if (currentAState && !lastAState) {
                gamePaused = !gamePaused;
            }
            lastAState = currentAState;

            if (!gamePaused)
            {
                // Control using Dreamcast D-pad
                if ((pad_state->buttons & CONT_DPAD_RIGHT) && (snake[0].speed.x == 0) && allowMove)
                {
                    snake[0].speed = (Vector2){ SQUARE_SIZE, 0 };
                    allowMove = false;
                }
                if ((pad_state->buttons & CONT_DPAD_LEFT) && (snake[0].speed.x == 0) && allowMove)
                {
                    snake[0].speed = (Vector2){ -SQUARE_SIZE, 0 };
                    allowMove = false;
                }
                if ((pad_state->buttons & CONT_DPAD_UP) && (snake[0].speed.y == 0) && allowMove)
                {
                    snake[0].speed = (Vector2){ 0, -SQUARE_SIZE };
                    allowMove = false;
                }
                if ((pad_state->buttons & CONT_DPAD_DOWN) && (snake[0].speed.y == 0) && allowMove)
                {
                    snake[0].speed = (Vector2){ 0, SQUARE_SIZE };
                    allowMove = false;
                }

                // Snake movement
                for (int i = 0; i < counterTail; i++) snakePosition[i] = snake[i].position;

                if ((framesCounter%5) == 0)
                {
                    for (int i = 0; i < counterTail; i++)
                    {
                        if (i == 0)
                        {
                            snake[0].position.x += snake[0].speed.x;
                            snake[0].position.y += snake[0].speed.y;
                            allowMove = true;
                        }
                        else snake[i].position = snakePosition[i-1];
                    }
                }

                // Wall behaviour
                if (((snake[0].position.x) > (SCREEN_WIDTH - offset.x)) ||
                    ((snake[0].position.y) > (SCREEN_HEIGHT - offset.y)) ||
                    (snake[0].position.x < 0) || (snake[0].position.y < 0))
                {
                    gameOver = true;
                }

                // Collision with yourself
                for (int i = 1; i < counterTail; i++)
                {
                    if ((snake[0].position.x == snake[i].position.x) && (snake[0].position.y == snake[i].position.y)) gameOver = true;
                }

                // Fruit position calculation
                if (!fruit.active)
                {
                    fruit.active = true;
                    fruit.position = (Vector2){ GetRandomValue(0, (SCREEN_WIDTH/SQUARE_SIZE) - 1)*SQUARE_SIZE + offset.x/2, 
                                              GetRandomValue(0, (SCREEN_HEIGHT/SQUARE_SIZE) - 1)*SQUARE_SIZE + offset.y/2 };

                    for (int i = 0; i < counterTail; i++)
                    {
                        while ((fruit.position.x == snake[i].position.x) && (fruit.position.y == snake[i].position.y))
                        {
                            fruit.position = (Vector2){ GetRandomValue(0, (SCREEN_WIDTH/SQUARE_SIZE) - 1)*SQUARE_SIZE + offset.x/2,
                                                      GetRandomValue(0, (SCREEN_HEIGHT/SQUARE_SIZE) - 1)*SQUARE_SIZE + offset.y/2 };
                            i = 0;
                        }
                    }
                }

                // Collision
                if ((snake[0].position.x < (fruit.position.x + fruit.size.x) && (snake[0].position.x + snake[0].size.x) > fruit.position.x) &&
                    (snake[0].position.y < (fruit.position.y + fruit.size.y) && (snake[0].position.y + snake[0].size.y) > fruit.position.y))
                {
                    snake[counterTail].position = snakePosition[counterTail - 1];
                    counterTail += 1;
                    fruit.active = false;
                }

                framesCounter++;
            }
        }
    }
    else
    {
        // Use B button to restart when game is over
        if (cont && pad_state && (pad_state->buttons & CONT_B))
        {
            InitGame();
            gameOver = false;
        }
    }
}

void DrawGame(void)
{
    BeginDrawing();

        ClearBackground(RAYWHITE);

        if (!gameOver)
        {
            // Draw grid lines
            for (int i = 0; i < SCREEN_WIDTH/SQUARE_SIZE + 1; i++)
            {
                DrawLineV((Vector2){SQUARE_SIZE*i + offset.x/2, offset.y/2}, 
                         (Vector2){SQUARE_SIZE*i + offset.x/2, SCREEN_HEIGHT - offset.y/2}, LIGHTGRAY);
            }

            for (int i = 0; i < SCREEN_HEIGHT/SQUARE_SIZE + 1; i++)
            {
                DrawLineV((Vector2){offset.x/2, SQUARE_SIZE*i + offset.y/2}, 
                         (Vector2){SCREEN_WIDTH - offset.x/2, SQUARE_SIZE*i + offset.y/2}, LIGHTGRAY);
            }

            // Draw snake
            for (int i = 0; i < counterTail; i++) DrawRectangleV(snake[i].position, snake[i].size, snake[i].color);

            // Draw fruit to pick
            DrawRectangleV(fruit.position, fruit.size, fruit.color);

            if (gamePaused) DrawText("GAME PAUSED", SCREEN_WIDTH/2 - MeasureText("GAME PAUSED", 40)/2, SCREEN_HEIGHT/2 - 40, 40, GRAY);
        }
        else DrawText("PRESS B TO PLAY AGAIN", SCREEN_WIDTH/2 - MeasureText("PRESS B TO PLAY AGAIN", 20)/2, SCREEN_HEIGHT/2 - 50, 20, GRAY);

    EndDrawing();
}

void UnloadGame(void)
{
    // Nothing to unload
}

void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}