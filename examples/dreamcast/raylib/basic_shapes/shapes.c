/*******************************************************************************************
*
*   raylib [shapes] example - Draw basic shapes 2d (rectangle, circle, line...)
*
*   Example originally created with raylib 1.0, last time updated with raylib 4.2
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2024 Ramon Santamaria (@raysan5)
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

    InitWindow(screenWidth, screenHeight, "raylib [shapes] example - basic shapes");
    
    float rotation = 0.0f;

   

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (flag)    // Detect window close with Start button
    {
        // Update
        //-----------------------------------------------------
        updateController();

         //----------------------------------------------------------------------------------
        rotation += 0.2f;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("some basic shapes available on raylib", 20, 20, 20, DARKGRAY);

            // Circle shapes and lines
            DrawCircle(screenWidth/5, 120, 35, DARKBLUE);
            DrawCircleGradient(screenWidth/5, 220, 60, GREEN, SKYBLUE);
            DrawCircleLines(screenWidth/5, 340, 80, DARKBLUE); //not supported on Dreamcast

            // Rectangle shapes and lines
            DrawRectangle(screenWidth/4*2 - 60, 100, 120, 60, RED);
            DrawRectangleGradientH(screenWidth/4*2 - 90, 170, 180, 130, MAROON, GOLD);
            DrawRectangleLines(screenWidth/4*2 - 40, 320, 80, 60, ORANGE);  // NOTE: Uses QUADS internally, not lines

            // Triangle shapes and lines
            DrawTriangle((Vector2){ screenWidth/4.0f *3.0f, 80.0f },
                         (Vector2){ screenWidth/4.0f *3.0f - 60.0f, 150.0f },
                         (Vector2){ screenWidth/4.0f *3.0f + 60.0f, 150.0f }, VIOLET);

            DrawTriangleLines((Vector2){ screenWidth/4.0f*3.0f, 160.0f },
                              (Vector2){ screenWidth/4.0f*3.0f - 20.0f, 230.0f },
                              (Vector2){ screenWidth/4.0f*3.0f + 20.0f, 230.0f }, DARKBLUE);//Not supported on Dreamcast

            // Polygon shapes and lines
            DrawPoly((Vector2){ screenWidth/4.0f*3, 330 }, 6, 80, rotation, BROWN);
            DrawPolyLines((Vector2){ screenWidth/4.0f*3, 330 }, 6, 90, rotation, BROWN); //Not supported on Dreamcast
            DrawPolyLinesEx((Vector2){ screenWidth/4.0f*3, 330 }, 6, 85, rotation, 6, BEIGE);

            // NOTE: We draw all LINES based shapes together to optimize internal drawing,
            // this way, all LINES are rendered in a single draw pass
            //DrawLine(18, 42, screenWidth - 18, 42, BLACK); //not supported on Dreamcast
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
