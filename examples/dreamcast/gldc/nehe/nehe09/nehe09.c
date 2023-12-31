/* 
   KallistiOS 2.0.0

   nehe09.c
   (c)2014 Josh Pearson
   (c)2001 Benoit Miller
   (c)2000 Jeff Molofee
*/

#include <kos.h>

#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glkos.h>

#define NUM_STARS 50        /* Number Of Stars To Draw */

typedef struct {        /* Create A Structure For Star */
    int r, g, b;        /* Stars Color */
    GLfloat dist,       /* Stars Distance From Center */
            angle;      /* Stars Current Angle */
} stars;

static stars star[NUM_STARS];   /* Need To Keep Track Of 'NUM_STARS' Stars */

static GLboolean twinkle;   /* Twinkling Stars */

static GLfloat zoom = -15.0f; /* Distance Away From Stars */
static GLfloat tilt = 90.0f; /* Tilt The View */
static GLfloat spin;        /* Spin Stars */

static GLuint loop;     /* General Loop Variable */
static GLuint texture[1];   /* Storage For One textures */

/* Load a PVR texture - located in pvr-texture.c */
extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

void draw_gl(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, texture[0]);

    for(loop = 0; loop < NUM_STARS; loop++) {
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, zoom);
        glRotatef(tilt, 1.0f, 0.0f, 0.0f);
        glRotatef(star[loop].angle, 0.0f, 1.0f, 0.0f);
        glTranslatef(star[loop].dist, 0.0f, 0.0f);
        glRotatef(-star[loop].angle, 0.0f, 1.0f, 0.0f);
        glRotatef(-tilt, 1.0f, 0.0f, 0.0f);

        if(twinkle) {
            glColor4ub(star[(NUM_STARS - loop) - 1].r, star[(NUM_STARS - loop) - 1].g, star[(NUM_STARS - loop) - 1].b, 255);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(-1.0f, -1.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(1.0f, -1.0f, 0.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(1.0f, 1.0f, 0.0f);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(-1.0f, 1.0f, 0.0f);
            glEnd();
        }

        glRotatef(spin, 0.0f, 0.0f, 1.0f);
        glColor4ub(star[loop].r, star[loop].g, star[loop].b, 255);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f, 1.0f, 0.0f);
        glEnd();

        spin += 0.01f;
        star[loop].angle += (float)(loop) / NUM_STARS;
        star[loop].dist -= 0.01f;

        if(star[loop].dist < 0.0f) {
            star[loop].dist += 5.0f;
            star[loop].r = rand() % 256;
            star[loop].g = rand() % 256;
            star[loop].b = rand() % 256;
        }
    }
}

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;
    GLboolean yp = GL_FALSE;

    printf("nehe09 beginning\n");

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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);

    for(loop = 0; loop < NUM_STARS; loop++) {
        star[loop].angle = 0.0f;
        star[loop].dist = ((float)(loop) / NUM_STARS) * 5.0f;
        star[loop].r = rand() % 256;
        star[loop].g = rand() % 256;
        star[loop].b = rand() % 256;
    }

    /* Set up the texture */
    texture[0] = glTextureLoadPVR("/rd/star.pvr", 0, 0);

    while(1) {
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        /* Check key status */
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state) {
            printf("Error reading controller\n");
            break;
        }

        if(state->buttons & CONT_START)
            break;

        if(state->buttons & CONT_DPAD_UP)
            tilt -= 0.5f;

        if(state->buttons & CONT_DPAD_DOWN)
            tilt += 0.5f;

        if(state->buttons & CONT_A)
            zoom -= 0.2f;

        if(state->buttons & CONT_B)
            zoom += 0.2f;

        if((state->buttons & CONT_Y) && !yp) {
            yp = GL_TRUE;
            twinkle = !twinkle;
        }

        if(!(state->buttons & CONT_Y))
            yp = GL_FALSE;

        /* Draw the GL "scene" */
        draw_gl();

        /* Finish the frame */
        glKosSwapBuffers();
    }

    return 0;
}


