/* 
   KallistiOS ##version##

   gl-arrays.c
   (c)2014 Josh Pearson

   Open GL example using Vertex Array Submission with Near Z Clipping.
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

typedef struct
{
    GLfloat position[3];
    GLfloat texCoord[2];
    GLuint  color;
} Vertex3tc; // 3 float vertex, textured, colored

static Vertex3tc * vertex;

static void SetVertex3tc(Vertex3tc * vertex, GLfloat x, GLfloat y, GLfloat z,
                         GLfloat u, GLfloat v, GLuint color)
{
    vertex->position[0] = x;
    vertex->position[1] = y;
    vertex->position[2] = z;  
    vertex->texCoord[0] = u;
    vertex->texCoord[1] = v; 
    vertex->color = color;    
}
static void buildDemoArray()
{
    vertex = malloc( sizeof( Vertex3tc ) * 6 );
    
    SetVertex3tc(&vertex[0], -100.0f, -10.0f, -100.0f, 0, 0, 0xFFFF0000);
    SetVertex3tc(&vertex[1], 100.0f, -10.0f, -100.0f, 1, 0, 0xFF00FF00);
    SetVertex3tc(&vertex[2], -100.0f, -10.0f, 100.0f, 0, 1, 0xFF0000FF);
    SetVertex3tc(&vertex[3], 100.0f, -10.0f, 100.0f, 1, 1, 0xFFFFFF00);
    SetVertex3tc(&vertex[4], -100.0f, -10.0f, 300.0f, 0, 0, 0xFFFF0000);
    SetVertex3tc(&vertex[5], 100.0f, -10.0f, 300.0f, 1, 0, 0xFF00FF00);       
}

static GLfloat rx = 1.0f;

/* Example using Open GL Vertex Array Submission. */
void RenderCallback(GLuint texID) {
    glEnable(GL_KOS_NEARZ_CLIPPING);
    
    /* Rotate the matrix to make sure transforms and clipping are working */
    glLoadIdentity();
    glRotatef(rx++, 0, 1, 0);

    /* Enable 2D Texturing and bind the Texture */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);
    
    /* Enable Vertex, Color and Texture Coord Arrays */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    /* Bind Vertex Array Data, Using Strided Vertices */
    glColorPointer(1, GL_UNSIGNED_INT, sizeof(Vertex3tc), &vertex[0].color);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex3tc), vertex[0].texCoord);
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex3tc), vertex[0].position);    
    
    /* Render the Submitted Vertex Data */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);

    /* Disable Vertex, Color and Texture Coord Arrays */
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

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
    gluPerspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Load a PVR texture to OpenGL */
    GLuint texID = glTextureLoadPVR("/rd/wp001vq.pvr", 0, 0);
    
    buildDemoArray();
    
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
