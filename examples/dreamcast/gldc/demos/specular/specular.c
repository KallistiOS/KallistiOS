/* 
   KallistiOS 2.0.0

   specular.c
   (c)2014 Josh Pearson

   Open GL Specular Lighting Example .
*/

#include <kos.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/glkos.h>

#include "vector.h"
#include "input.h"
#include "texture.h"
#include "timer.h"
#include "font.h"

/* Simple OpenGL example to demonstrate specular lighting, and Z clipping.
   Specular Lighting is dependent upon the camera position,
   so here you can move the camera in a fps style.

** User controls **
   Hold Left trigger, then press A,B,X, or Y to Enable Light1->4
   Hold Right trigger, then press A,B,X, or Y to Disable Light1->4
   D-pad to rotate camera
   A,B,X,Y to move camera
   Start to exit

   As Vertex Clipping and Lighting is being applied using immediate mode,
   this really is a brute-force approach to the vertex submission pipeline.
   It would be much more optimized to build the meshes, and submit
   as arrays...
 */

static GLfloat z = -5.0f;   /* Depth Into The Screen */
static GLuint texture[2];   /* Storage For Two Textures */

static vector3f      up = { 0,  1,  0 },
                     camFrom = { -74, 10, 0.0 },
                     camTo = { -74, 10, 10.0 };

typedef struct {
    float min,
          max,
          last,
          avg;
    unsigned int frame;
} Fps;

static Fps fps = {45, 0, 0, 0, 0};

static Font *font;

void glSetCameraPosition(vector3f campos, vector3f camdst, vector3f up) {
    /* Set up GL Render Stack based on Camera Perspective */
    glLoadIdentity();
    gluLookAt(campos[0], campos[1], campos[2],
              camdst[0], camdst[1], camdst[2],
              up[0], up[1], up[2]);
}

void draw_gl_cube(float x, float y, float z, unsigned char a,
                  unsigned char r, unsigned char g, unsigned char b) {
    glPushMatrix();

    glDisable(GL_LIGHTING);

    //glColor1ui(color);
    glColor4ub(a, r, g, b);

    glTranslatef(x, y, z);

    glBindTexture(GL_TEXTURE_2D, texture[0]);

    glBegin(GL_QUADS);
    /* Front Face */
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    /* Back Face */
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    /* Top Face */
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    /* Bottom Face */
    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    /* Right face */
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    /* Left Face */
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glEnd();

    glEnable(GL_LIGHTING);

    glPopMatrix();
}

#define LGAP 13

void draw_gl(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, z);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}

void draw_gl2(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, -z);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}


void draw_gl3(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, -15);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}


void draw_gl4(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, 15);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}

void draw_gl5(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, -25);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}


void draw_gl6(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, 25);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}


void draw_gl7(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, -35);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}


void draw_gl8(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, 35);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}


void draw_gl9(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, -45);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}


void draw_gl10(void) {
    glPushMatrix();

    glTranslatef(0, 0.0f, 45);
    int l;

    for(l = 0; l < LGAP; l++) {
        glTranslatef(z, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glBegin(GL_QUADS);
        /* Front Face */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        /* Back Face */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        /* Top Face */
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        /* Bottom Face */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        /* Right face */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f,  1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(1.0f, -1.0f,  1.0f);
        /* Left Face */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f,  1.0f, -1.0f);
        glEnd();
    }

    glPopMatrix();
}

void GPU_Stats(void) {
    pvr_stats_t stats;
    pvr_get_stats(&stats);

    if(stats.frame_rate > 1) {
        ++fps.frame;

        fps.last = stats.frame_rate;

        if(fps.last < fps.min) fps.min = fps.last;

        if(fps.last > fps.max) fps.max = fps.last;

        fps.avg += fps.last;

        if(fps.frame % 60 == 0) {
            printf("PVR FPS MIN: %.2f | MAX: %.2f | LAST: %.2f | AVG: %.2f\n",
                   (double)fps.min, (double)fps.max, (double)fps.last, (double)fps.avg / fps.frame);
        }
    }
    else
        printf("PVR: WATING FOR STATS\n");
}

static unsigned char LE[8] = {0, 1, 0, 0, 0, 0, 0, 0};
int InputCb(void) {
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        cont_state_t *state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            return 1;

        if(state->buttons & CONT_START)
            return 0;

        if(state->ltrig > 0) {
            if(state->buttons & CONT_A)
                LE[0] = 1;

            if(state->buttons & CONT_B)
                LE[2] = 1;

            if(state->buttons & CONT_Y)
                LE[3] = 1;

            if(state->buttons & CONT_X)
                LE[4] = 1;
        }

        if(state->rtrig > 0) {
            if(state->buttons & CONT_A)
                LE[0] = 0;

            if(state->buttons & CONT_B)
                LE[2] = 0;

            if(state->buttons & CONT_Y)
                LE[3] = 0;

            if(state->buttons & CONT_X)
                LE[4] = 0;
        }
    }

    return 1;
}

int main() {
    /* Get basic stuff initialized */
    glKosInit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* Enable Near-Z Clipping */
    glEnable(GL_NEARZ_CLIPPING_KOS);

    /* Enable Lighting and GL_LIGHT0 */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* Set Light and Material Parameters */
    float lp[4] = { 0.0, 10.0, 0.0, 0.0 };
    float lp2[4] = { 0.0, 10.0, 0.0, 0.0 };
    float lc[4] = { -50.0, 10.0, 0.0, 0.0 };
    float lcolor[3] = { 0xCC, 0x00, 0x00 };
    float lcolor2[3] = { 0x00, 0x00, 0xCC };
    float lcolors[3] = { 0x99, 0x99, 0x99 };
    glLightfv(GL_LIGHT0, GL_POSITION, lp);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lcolor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lcolors);
    glLightfv(GL_LIGHT1, GL_POSITION, lp2);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lcolor2);
    glLightfv(GL_LIGHT1, GL_SPECULAR, lcolors);

    float l3c[3] = { 0.5, 0.5, 0.0 };
    float l3p[4] = { 0, -10, 0, 0 };
    float l3d[3] = { 0, 1, 0 };
    glLightfv(GL_LIGHT2, GL_POSITION, l3p);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, l3c);
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, l3d);

    float l4c[3] = { 0.0, 0.5, 0.5 };
    float l4p[4] = { -50, -10, 0, 0 };
    float l4d[3] = { 0, 1, 0 };
    glLightfv(GL_LIGHT3, GL_POSITION, l4p);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, l4c);
    glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, l4d);


    float l5c[3] = { 0.0, 0.8, 0.0 };
    float l5p[4] = { -50, -10, 50, 0 };
    float l5d[3] = { 0, 1, 0 };
    glLightfv(GL_LIGHT4, GL_POSITION, l5p);
    glLightfv(GL_LIGHT4, GL_DIFFUSE, l5c);
    glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, l5d);

    /* Set Material Parameters */
    float Mat_Specular[3] = { 0.6, 0.6, 0.6 };
    glLightfv(GL_LIGHT0, GL_SPECULAR, Mat_Specular);
    glMaterialfv(GL_FRONT, GL_SPECULAR, Mat_Specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);

    /* Set up the textures */
    texture[0] = glTextureLoadPVR("/rd/brick_w1.pvr", 0, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    texture[1] = glTextureLoadPVR("/rd/FONT0.pvr", 0, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    font = FontInit(512, 512, 10, 10, 0xFF, 0xFF, 0xFF, 0xFF);


    char CPU_TIME[64] = { '\n' };
    char GPU_TIME[64] = { '\n' };

    while(1) {
        InputCallback(camFrom, camTo);

        GLuint start = GetTime();

        if (!InputCb())
           return 0;

        glSetCameraPosition(camFrom, camTo, up);

        /* Draw the GL "scene" - Very UGLY User Code! Only Demonstration of API */
        if(LE[0]) {
            glEnable(GL_LIGHT0);
            draw_gl_cube(lp[0], lp[1], lp[2], 0xFF, lcolor[0], lcolor[1], lcolor[2]);
            glLightfv(GL_LIGHT0, GL_POSITION, lp);
        }
        else
            glDisable(GL_LIGHT0);

        if(LE[1]) {
            glEnable(GL_LIGHT1);
            draw_gl_cube(lp2[0], lp2[1], lp2[2], 0xFF, lcolor2[0], lcolor2[1], lcolor2[2]);
        }
        else
            glDisable(GL_LIGHT1);

        if(LE[2]) {
            glEnable(GL_LIGHT2);
            draw_gl_cube(l3p[0], l3p[1], l3p[2], 0xFF, l3c[0], l3c[1], l3c[2]);
            glLightfv(GL_LIGHT2, GL_POSITION, l3p);
        }
        else
            glDisable(GL_LIGHT2);

        if(LE[3]) {
            glEnable(GL_LIGHT3);
            draw_gl_cube(l4p[0], l4p[1], l4p[2], 0xFF, l4c[0], l4c[1], l4c[2]);
        }
        else
            glDisable(GL_LIGHT3);

        if(LE[4]) {
            glEnable(GL_LIGHT4);
            draw_gl_cube(l5p[0], l5p[1], l5p[2], 0xFF, l5c[0], l5c[1], l5c[2]);
            glLightfv(GL_LIGHT4, GL_POSITION, l5p);
        }
        else
            glDisable(GL_LIGHT4);

        draw_gl();
        draw_gl2();
        draw_gl3();
        draw_gl4();
        draw_gl5();
        draw_gl6();
        draw_gl7();
        draw_gl8();
        draw_gl9();
        draw_gl10();

        /* Render the 2D Overlay */
        glDisable(GL_LIGHTING);
        glDisable(GL_NEARZ_CLIPPING_KOS);

        char str[64];
        sprintf(str, "FPS: %.2f\n", (double)fps.avg / fps.frame);

        glBindTexture(GL_TEXTURE_2D, texture[1]);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        glBegin(GL_QUADS);
        FontPrintString(font, str, 16, 448 - 16, 16, 16);
        FontPrintString(font, CPU_TIME, 16, 16, 16, 16);
        FontPrintString(font, GPU_TIME, 16, 34, 16, 16);
        glEnd();

        glDisable(GL_BLEND);

        glEnable(GL_DEPTH_TEST);

        glEnable(GL_NEARZ_CLIPPING_KOS);
        glEnable(GL_LIGHTING);

        sprintf(CPU_TIME, "OpenGL CPU TIME: %i\n", GetTime() - start);
        start = GetTime();

        /* Finish the frame */
        glKosSwapBuffers();

        sprintf(GPU_TIME, "OpenGL GPU TIME: %i\n", GetTime() - start);

        GPU_Stats();

        vec3f_rotd_xz(lp[0], lp[1], lp[2], lc[0], lc[1], lc[2], 0.5f);
    }

    return 0;
}

