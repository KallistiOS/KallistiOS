/* 
   KallistiOS ##version##

   gl-elements.c
   (c)2014 Josh Pearson

   Open GL example using Vertex Array Submission and GL Lighting.
*/

#include <stdio.h>
#include <stdlib.h>

#include <KGL/gl.h>
#include <KGL/glu.h>
#include <KGL/glut.h>

#include <kos/init.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

/* Load a PVR texture - located in pvr-texture.c */
extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

GLfloat VERTEX_ARRAY[4 * 3 * 1] = { -100.0f, -10.0f, 100.0f,
                                    100.0f, -10.0f, 100.0f,
                                    100.0f, -10.0f, -100.0f,
                                    -100.0f, -10.0f, -100.0f
                                  };

GLfloat TEXCOORD_ARRAY[4 * 2 * 1] = { 0, 0,
                                      1, 0,
                                      1, 1,
                                      0, 1
                                    };

GLfloat NORMAL_ARRAY[4 * 3 * 1] = { 0, -1, 0,
                                    0, 1, 0,
                                    0, 1, 0,
                                    0, 1, 0
                                  };

GLubyte INDEX_ARRAY[4 * 1] = { 0, 1, 2, 3 };

/* Example using Open GL Vertex Array Submission using glDrawElements().*/
static float rx = 1.0f;
void RenderCallback(GLuint texID) {
    /* Enable KOS Near Z Vertex Clipping */
    glEnable(GL_KOS_NEARZ_CLIPPING);

    /* Set up Light1 - color it red */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    float lp[4] = { 0.0, 200.0, 0.0, 0.0 };
    float lc[3] = { 1.0f, 0.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lp);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lc);

    /* Load Identity and rotate it a bit */
    glLoadIdentity();
    glRotatef(rx++, 0, 1, 0);

    /* Enable 2D Texturing and bind the Texture */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);

    /* Bind Array Data */
    glNormalPointer(GL_FLOAT, 0, NORMAL_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, TEXCOORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, VERTEX_ARRAY);

    /* Render the Submitted Vertex Data */
    glDrawElements(GL_QUADS, 4 * 1, GL_UNSIGNED_BYTE, INDEX_ARRAY);

    /* Disable GL Vertex Lighting */
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);

    /* Disable GL KOS Near Z Vertex Clipping */
    glDisable(GL_KOS_NEARZ_CLIPPING);
}

static int check_start(void) {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        state = (cont_state_t *)maple_dev_status(cont);

        if(state)
            return state->buttons & CONT_START;
    }

    return 0;
}

int main(int argc, char **argv) {
    /* Notice we do not init the PVR here, that is handled by Open GL */
    glKosInit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, vid_mode->width / vid_mode->height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* Load a PVR texture to OpenGL */
    GLuint texID = glTextureLoadPVR("/rd/wp001vq.pvr", 0, 0);

    while(1) {
        if(check_start())
            break;

        /* Draw the "scene" */
        RenderCallback(texID);

        /* Finish the frame - Notice there is no glKosBegin/FinshFrame */
        glutSwapBuffers();
    }

    return 0;
}
