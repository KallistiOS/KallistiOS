/*
   KallistiOS 2.0.0

   gl-mipmap.c
   (c)2014 Josh Pearson

   Open GL Mip-Map example.

   Controls:
    X = Use MipMapped Texture
    Y = Use Base Texture
    D-pad UP = Scale image size up
    D-pad DOWN = Scale image size down
    Start = Exit
*/

#include <kos.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glkos.h>

/* Load a PVR texture - located in pvr-texture.c */
extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

/* Input Callback Return Values */
#define INP_RESIZE_UP   1
#define INP_RESIZE_DOWN 2
#define INP_USE_MIP_MAP 3
#define INP_NO_MIP_MAP  4
#define INP_EXIT        5

/* Simple Input Callback with a return value */
int InputCallback(void) {
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        cont_state_t *state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            return 0;

        if(state->buttons & CONT_START)
            return INP_EXIT;

        if(state->buttons & CONT_DPAD_UP)
            return INP_RESIZE_UP;

        if(state->buttons & CONT_DPAD_DOWN)
            return INP_RESIZE_DOWN;

        if(state->buttons & CONT_X)
            return INP_USE_MIP_MAP;

        if(state->buttons & CONT_Y)
            return INP_NO_MIP_MAP;
    }

    return 0;
}

/* Very Basic Open GL Initialization for 2D rendering */
void RenderInit(void) {
    glKosInit(); /* GL Will Initialize the PVR */

    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);
}

/* Render a Textured Quad of given texture ID, width, and height */
void RenderTexturedQuadCentered(GLuint texID, GLfloat width, GLfloat height) {
    GLfloat x1 = (vid_mode->width - width) / 2.0;
    GLfloat x2 = x1 + width;
    GLfloat y1 = (vid_mode->height - height) / 2.0;
    GLfloat y2 = y1 + height;

    glBindTexture(GL_TEXTURE_2D, texID);

    glBegin(GL_QUADS);

    glTexCoord2f(0, 0);
    glVertex2f(x1, y1);

    glTexCoord2f(1, 0);
    glVertex2f(x2, y1);

    glTexCoord2f(1, 1);
    glVertex2f(x2, y2);

    glTexCoord2f(0, 1);
    glVertex2f(x1, y2);

    glEnd();
}

int main(int argc, char **argv) {
    printf("OpenGL MipMap Example (C) 2014 PH3NOM\n");

    RenderInit(); /* Initialize Open GL */

    /* Load a PVR texture to OpenGL with No MipMap */
    GLuint texID0 = glTextureLoadPVR("/rd/MP_512.pvr", 0, 0);

    /* Load a PVR texture to OpenGL with No MipMap, but use Open GL to create the MipMap */
    GLuint texID1 = glTextureLoadPVR("/rd/MP_512.pvr", 0, 1);

    GLuint curTexID = texID0;
    GLfloat width = 480, height = 480;

    while(1) {
        switch(InputCallback()) {
            case INP_RESIZE_UP:
                ++width;
                ++height;
                break;

            case INP_RESIZE_DOWN:
                if(width > 1 && height > 1) {
                    --width;
                    --height;
                }

                break;

            case INP_NO_MIP_MAP:
                curTexID = texID0;
                break;

            case INP_USE_MIP_MAP:
                curTexID = texID1;
                break;

            case INP_EXIT:
                return 0;
        }

        RenderTexturedQuadCentered(curTexID, width, height);

        glKosSwapBuffers();
    }

    return 0;
}
