/*
   KallistiOS 2.0.0

   vq-example.c
   (c)2014 Josh Pearson
   (c)2002 Gil Megidish
   (c)2001 Benoit Miller

   This is a modified nehe06.c that shows the capabilities of
   VQ compression.

   Texture is copyright (c)2002 Mayang Murni Adnin; for more
   incredible textures, go to www.mayang.com
*/

#include <kos.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/glkos.h>

GLfloat xrot;   /* X Rotation */
GLfloat yrot;   /* Y Rotation */
GLfloat zrot;   /* Z Rotation */

GLuint texture[1];

/* external vq-texture storage */
extern unsigned char fruit[];
extern unsigned char fruit_end[];

/* Load a texture with glCompressedTexImage2D */
static int loadtxr(void) {

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, texture[0]);

    glCompressedTexImage2D(GL_TEXTURE_2D,  /* This must be GL_TEXTURE_2D */
                           0,             /* 0 = Texture does not contain Mip-Maps */
                           GL_COMPRESSED_RGB_565_VQ_TWID_KOS, /* GL Compressed Color Format */
                           512,           /* Texture Width */
                           512,           /* Texture Height */
                           0,             /* This bit must be set to 0 */
                           (uintptr_t)fruit_end - (uintptr_t)fruit, /* Compressed Texture Size*/
                           fruit);       /* Address of texture data in RAM: OpenGL will load the texture into VRAM for you.
                                            Because of this, make sure to call glDeleteTextures() as needed, as that will
                                            free the VRAM allocated for the texture. */
    return 0;
}

void draw_gl(void) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);

    glRotatef(xrot, 1.0f, 0.0f, 0.0f);
    glRotatef(yrot, 0.0f, 1.0f, 0.0f);
    glRotatef(zrot, 0.0f, 0.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, texture[0]);

    glBegin(GL_QUADS);

    /* Front Face */
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);

    /* Back Face */
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);

    /* Top Face */
    glColor3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);

    /* Bottom Face */
    glColor3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);

    /* Right face */
    glColor3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glColor3f(0.3f, 0.5f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glColor3f(1.0f, 0.3f, 0.5f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glColor3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);

    /* Left Face */
    glColor3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glColor3f(0.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);

    glEnd();

    xrot += 0.3f;
    yrot += 0.2f;
    zrot += 0.4f;
}

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;

    /* Get basic stuff initialized */
    glKosInit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* Set up the texture */
    if(loadtxr() < 0) {
        printf("loadtxr() failed\n");
        return 0;
    }

    while(1) {
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(!cont) {
            printf("No controllers connected\n");
            break;
        }

        /* Check key status */
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state) {
            printf("Error reading controller\n");
            break;
        }

        if(state->buttons & CONT_START)
            break;

        /* Draw the GL "scene" */
        draw_gl();

        /* Finish the frame */
        glKosSwapBuffers();
    }

    glDeleteTextures(1, texture);

    return 0;
}
