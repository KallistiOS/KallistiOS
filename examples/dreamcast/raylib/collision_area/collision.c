/*******************************************************************************************
*
*   raylib [shapes] example - collision area
*
*   Example originally created with raylib 2.5, last time updated with raylib 2.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2013-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include <kos.h>
#include <raylib.h>

#define ATTR_DREAMCAST_WIDTH 640
#define ATTR_DREAMCAST_HEIGHT 480
bool flag=true;
bool xflag=false;
maple_device_t *cont;
cont_state_t *pad_state;
int x;
int y;

void updateController()
{
    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont)
    {
        pad_state = (cont_state_t *)maple_dev_status(cont);

        if(!pad_state)
        {
            printf("Error reading controller\n");
        }

        if(pad_state->buttons & CONT_START)
        {
            flag=0;
        }

        if(pad_state->buttons & CONT_DPAD_UP)
        {
            y=y-10;
        }

        if(pad_state->buttons & CONT_DPAD_DOWN)
        {
            y=y+10;
        }

        if(pad_state->buttons & CONT_DPAD_RIGHT)
        {
            x=x+10;
        }

        if(pad_state->buttons & CONT_DPAD_LEFT)
        {
            x=x-10;
        }


        
        if(pad_state->buttons & CONT_A) 
        {
            xflag = !xflag;
        }
    }
}

bool initApp()
{
    return true;
}
void finishApp()
{
    
    
}
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = ATTR_DREAMCAST_WIDTH;
    const int screenHeight = ATTR_DREAMCAST_HEIGHT;

    InitWindow(screenWidth, screenHeight, "raylib [shapes] example - raylib collision area");
    

    // Box A: Moving box
    Rectangle boxA = { 10, GetScreenHeight()/2.0f - 50, 200, 100 };
    int boxASpeedX = 4;

    // Box B: Mouse moved box
   
    x=screenWidth/2;
    y=screenHeight/2;

    Rectangle boxCollision = { 0 }; // Collision rectangle
    Rectangle boxB= { GetScreenWidth()/2.0f - 30, GetScreenHeight()/2.0f - 30, 60, 60 };

    int screenUpperLimit = 40;      // Top menu limits

    
    bool collision = false;         // Collision detection

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (flag)    // Detect window close button or ESC key
    {
        // Update
        //-----------------------------------------------------
        updateController();


        // Move box if not paused
        if (!xflag) boxA.x += boxASpeedX;

        // Bounce box on x screen limits
        if (((boxA.x + boxA.width) >= GetScreenWidth()) || (boxA.x <= 0)) boxASpeedX *= -1;

        // Update player-controlled-box (box02)
        boxB.x = x - boxB.width/2;
        boxB.y = y - boxB.height/2;

        // Make sure Box B does not go out of move area limits
        if ((boxB.x + boxB.width) >= GetScreenWidth()) boxB.x = GetScreenWidth() - boxB.width;
        else if (boxB.x <= 0) boxB.x = 0;

        if ((boxB.y + boxB.height) >= GetScreenHeight()) boxB.y = GetScreenHeight() - boxB.height;
        else if (boxB.y <= screenUpperLimit) boxB.y = (float)screenUpperLimit;

        // Check boxes collision
        collision = CheckCollisionRecs(boxA, boxB);

        // Get collision rectangle (only on collision)
        if (collision) boxCollision = GetCollisionRec(boxA, boxB);

        //-----------------------------------------------------

        // Draw
        //-----------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawRectangle(0, 0, screenWidth, screenUpperLimit, collision? RED : BLACK);

            DrawRectangleRec(boxA, GOLD);
            DrawRectangleRec(boxB, BLUE);

            if (collision)
            {
                // Draw collision area
                DrawRectangleRec(boxCollision, LIME);

                // Draw collision message
                DrawText("COLLISION!", GetScreenWidth()/2 - MeasureText("COLLISION!", 20)/2, screenUpperLimit/2 - 10, 20, BLACK);

                // Draw collision area
                DrawText(TextFormat("Collision Area: %i", (int)boxCollision.width*(int)boxCollision.height), GetScreenWidth()/2 - 100, screenUpperLimit + 10, 20, BLACK);
            }

            // Draw help instructions
            DrawText("Press A to PAUSE/RESUME", 20, screenHeight - 35, 20, LIGHTGRAY);

            DrawFPS(10, 10);

        EndDrawing();
        //-----------------------------------------------------
    }

    // De-Initialization
    //---------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //----------------------------------------------------------
    
    finishApp();
    return 0;
}
