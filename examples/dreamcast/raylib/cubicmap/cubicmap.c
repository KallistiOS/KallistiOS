/*******************************************************************************************
*
*   raylib [models] example - Cubicmap loading and drawing
*
*   Example originally created with raylib 1.8, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2015-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/


#include <kos.h>
#include <raylib.h>

#define ATTR_ORBIS_WIDTH 640
#define ATTR_ORBIS_HEIGHT 480
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
    const int screenWidth = ATTR_ORBIS_WIDTH;
    const int screenHeight = ATTR_ORBIS_HEIGHT;

    InitWindow(screenWidth, screenHeight, "raylib [models] example - cubicmap");
    
    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = (Vector3){ 16.0f, 14.0f, 16.0f };     // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };          // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };              // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                    // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;                 // Camera projection type

    Image image = LoadImage("/rd/cubicmap.png");      // Load cubicmap image (RAM)
    Texture2D cubicmap = LoadTextureFromImage(image);       // Convert image to texture to display (VRAM)

    Mesh mesh = GenMeshCubicmap(image, (Vector3){ 1.0f, 1.0f, 1.0f });
    Model model = LoadModelFromMesh(mesh);

    // NOTE: By default each cube is mapped to one part of texture atlas
    Texture2D texture = LoadTexture("/rd/cubicmap_atlas.png");    // Load map texture
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;    // Set map diffuse texture

    Vector3 mapPosition = { -16.0f, 0.0f, -8.0f };          // Set model position

    UnloadImage(image);     // Unload cubesmap image from RAM, already uploaded to VRAM

   

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (flag)    // Detect window close button or ESC key
    {
        // Update
        //-----------------------------------------------------
        updateController();

        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_ORBITAL);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                DrawModel(model, mapPosition, 1.0f, WHITE);

            EndMode3D();

            DrawTextureEx(cubicmap, (Vector2){ screenWidth - cubicmap.width*4.0f - 20, 20.0f }, 0.0f, 4.0f, WHITE);
            DrawRectangleLines(screenWidth - cubicmap.width*4 - 20, 20, cubicmap.width*4, cubicmap.height*4, GREEN);

            DrawText("cubicmap image used to", 658, 90, 10, GRAY);
            DrawText("generate map 3d model", 658, 104, 10, GRAY);

            DrawFPS(10, 10);

        EndDrawing();

    
        //-----------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(cubicmap);    // Unload cubicmap texture
    UnloadTexture(texture);     // Unload map texture
    UnloadModel(model);         // Unload map model
 
    CloseWindow();        // Close window and OpenGL context
    //----------------------------------------------------------
    
    finishApp();
    return 0;
}





