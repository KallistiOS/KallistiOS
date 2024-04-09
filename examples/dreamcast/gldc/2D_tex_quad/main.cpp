/*
    KallistiOS ##version##
    examples/dreamcast/gldc/2D_tex_quad/main.cpp
    Copyright (C) 2024 Jason Rost (Oni Enzeru)

    Example of 2D Orthographic perspective rendering
    of a textured quad using PNGs and GLdc.
*/

#include <kos.h>
#include <kos/img.h>

// ..:: GLdc ::..
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/glkos.h>

// ..:: STB_Image from KOS-Port
#include <stb_image/stb_image.h>


void GL_Init(uint16_t w, uint16_t h) {
    // Set "background" color to light blue
    glClearColor(0.10f, 0.5f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);

    // Expect Counter Clockwise vertex order
    glFrontFace(GL_CCW);

    // Enable Transparancy
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Depth test not needed for 2D
    glDisable(GL_DEPTH_TEST);

    // Set Orthographic ( 2D ) Camera
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Screen / Camera coords
    //      Left   Right    Bottom  Top      Near   Far
    glOrtho(0.0f, (float)w, 0.0f,  (float)h, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char **argv) {
    // Setup for Controller
    maple_device_t  *cont;
    cont_state_t    *state;
    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    // Texture ID
    GLuint texture;

    // Initialize KOS
    dbglog_set_level(DBG_WARNING);
    printf("\n..:: 2D Textured Quad Example - Start ::..\n");
    glKosInit();

    // Set screen size and init GLDC
    GL_Init(640, 480);

    // Generate a texture and link it to our ID
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture parameters best suited for Pixel Art ( Hard edge no repeat for scaling )
    // Prevents blur on scale.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Scale down filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Scale up filter

    // Load PNG via stb_image
    int width, height, nr_channels;
    //stbi_set_flip_vertically_on_load(true); // Flip img onload due to GL expecting top left (0,0)
    unsigned char *data = stbi_load("/rd/crate.png", &width, &height, &nr_channels, 0);

    // texture load debug
    printf("..:: STB_IMAGE Data ::..\n");
    printf("nr_channels: %d\n", nr_channels);
    printf("width:       %d\n", width);
    printf("height:      %d\n", height);

    // If data is loaded, apply texture formats and assign the data
    if(data) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     width,
                     height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     data);

        // Currently REQUIRED for OpenGLdc w/PNGs on Dreamcast
        glGenerateMipmapEXT(GL_TEXTURE_2D);
    }
    else {
        printf("Failed to load texture!\n");
    }

    // Cleanup
    stbi_image_free(data);

    // ..:: Output any GL Errors
    GLenum error = glGetError();
    if(error != GL_NO_ERROR) {
        printf("OpenGL error: %x\n", error);
    }
    // ..:: Vertex / UV Data setup
   float uv[4][2]  = {
                        // UVs are in reverse Y order to flip image
                        // GL expects 0,0 to be the bottom left corner of an image while
                        // real world coordinates are typically top left.
                        // alt- use: stbi_set_flip_vertically_on_load(true); before loading texture.
                        // X    // Y
                        {0.0f, 0.0f},
                        {1.0f, 0.0f},
                        {1.0f, 1.0f},
                        {0.0f, 1.0f}
                    };

    float xyz[4][3] = {
                        // Remember these are Counter-clockwise
                        /*  (-1,-1)-----( 0,-1)-----( 1,-1)
                               |           |           |
                               |           |           |
                               |           |           |
                            (-1, 0)-----( 0, 0)-----( 1, 0)
                               |           |           |
                               |           |           |
                               |           |           |
                            (-1, 1)-----( 0, 1)-----( 1, 1)
                        */
                        // X      Y      Z
                        { -1.0f,  1.0f,  1.0f },    // Bottom Left
                        {  1.0f,  1.0f,  1.0f },    // Bottom Right
                        {  1.0f, -1.0f,  1.0f },    // Top Right
                        { -1.0f, -1.0f,  1.0f }     // Top Left
                      };
    // ..:: Setup some variables to manipulate position and rotation of our quad
    float pos_x = 320.0f;
    float pos_y = 240.0f;
    float rot   = 0.0f;

    while(1) {
        //..:: Check Controller Input
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state) {
            printf("Error reading controller\n");
            break;
        }

        // Close program on Start Button
        if(state->start)
            break;

        //..:: Rotation on Triggers
        // Rotate CCW
        if(state->ltrig >= 255) {
            rot += 4.0f;
        }

        // Rotate CW
        if(state->rtrig >= 255) {
            rot -= 4.0f;
        }

        //..:: Scale on Y / B
        // Scale up
        if(state->y) {
            width = height += 4.0f;
        }

        // Scale down
        if(state->b) {
            width = height -= 4.0f;
        }

        // ..:: Begin GL Drawing
        glClearColor(0.10f, 0.5f, 1.0f, 1.0f);              // Sets background Color
        glClear(GL_COLOR_BUFFER_BIT);                     // Clears screen to that color

        pos_x += state->joyx * 0.05;
        pos_y -= state->joyy * 0.05;

        // Apply texture data to all draw calls until next bind
        glBindTexture(GL_TEXTURE_2D, texture);

        glPushMatrix();

            glLoadIdentity();

            // Matrix transforms are applied in reverse order.
            // Order should ALWAYS be:
            // 1) Translate
            // 2) Rotate
            // 3) Scale
            // Scaling "First" ( last in code ) allows all other transforms to be scaled as well.

            // Move quad to screen position
            //           X      Y      Z
            glTranslatef(pos_x, pos_y, 0.0f);
            //        val  X-axis   Y-axis   Z-axis
            glRotatef(rot, 0.0f,    0.0f,    1.0f);   // rotate only on enabled Axis (1.0f)
            //      tex_w   tex_h   Z doesn't change
            glScalef(width, height, 0.0f );

            glBegin(GL_QUADS);
                // Remember these are Counter-clockwise
                // Bottom Left
                glTexCoord2fv(&uv[0][0]);
                glVertex3f(xyz[0][0], xyz[0][1], xyz[0][2]);

                // Bottom Right
                glTexCoord2fv(&uv[1][0]);
                glVertex3f(xyz[1][0], xyz[1][1], xyz[1][2]);

                // Top Right
                glTexCoord2fv(&uv[2][0]);
                glVertex3f(xyz[2][0], xyz[2][1], xyz[2][2]);

                // Top Left
                glTexCoord2fv(&uv[3][0]);
                glVertex3f(xyz[3][0], xyz[3][1], xyz[3][2]);

            glEnd();

        glPopMatrix();

        // Finish the frame and flush to screen
        glKosSwapBuffers();
    }

    // ..:: Cleanup Memory
    glDeleteTextures(1, &texture);

    return 0;
}
