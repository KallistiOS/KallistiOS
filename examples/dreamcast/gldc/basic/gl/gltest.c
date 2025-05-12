/* 
   KallistiOS 2.0.0

   gltest.c
   (c)2014 Josh Pearson.
   (c)2001 Megan Potter
*/

#include <kos.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glkos.h>

/*

This is a really simple KallistiGL example. It shows off several things:
basic matrix control, perspective, and controlling the image with maple input.

Thanks to NeHe's tutorials for the texture.

*/

/*

This example demonstrates some of the differences between KGL and OpenGL.

Notice how we actually call glEnable(GL_BLEND) to enable transparency.
This remains in effect untill we call glDisable(GL_BLEND).
In OpenGL, there is no glKosFinishList() like in KGL.

Also, in OpenGL we call glutSwapBuffers() to flush the vertices to the GPU.
There is no glKosBeginFrame() or glKosFinishFrame() like in KGL.

Another thing, in OpenGL there is no GL_KOS_AUTO_UV, so forget about that.

Finally, OpenGL will initialize the PVR for you, so do not call pvr_init.
OpenGL is initialized with glKosInit().

*/

extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

/* Draw a cube centered around 0,0,0. Note the total lack of glTexCoord2f(),
   even though the cube is in fact getting textured! This is KGL's AUTO_UV
   feature: turn it on and it assumes you're putting on the texture on
   the quad in the order 0,0; 1,0; 1,1; 0,1. This only works for quads. */
void cube(float r) {
    glRotatef(r, 1.0f, 0.0f, 1.0f);

    glBegin(GL_QUADS);

    /* Front face */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);

    /* Back face */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);

    /* Left face */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);

    /* Right face */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);

    /* Top face */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);

    /* Bottom face */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glEnd();
}

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;
    float   r = 0.0f;
    float   dr = 2;
    float   z = -14.0f;
    int trans = 0;
    pvr_stats_t stats;

    printf("gltest beginning\n");

    /* Get basic stuff initialized */
    glKosInit();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    /* Expect CW verts */
    glFrontFace(GL_CW);

    /* Set Blend Mode */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Load a texture */
    glTextureLoadPVR("/rd/glass.pvr", 0, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glClearColor(0.3f, 0.4f, 0.5f, 1.0f);

    while(1) {
    	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        /* Check key status */
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(cont) {
            state = (cont_state_t *)maple_dev_status(cont);

            if(!state) {
                printf("Error reading controller\n");
            }

            if(state->buttons & CONT_START)
                break;

            if(state->buttons & CONT_DPAD_UP)
                z -= 0.1f;

            if(state->buttons & CONT_DPAD_DOWN)
                z += 0.1f;

            if(state->buttons & CONT_DPAD_LEFT) {
                /* If manual rotation is requested, then stop
                   the automated rotation */
                dr = 0.0f;
                r -= 2.0f;
            }

            if(state->buttons & CONT_DPAD_RIGHT) {
                dr = 0.0f;
                r += 2.0f;
            }

            if(state->buttons & CONT_A) {
                /* This weird logic is to avoid bouncing back
                   and forth before the user lets go of the
                   button. */
                if(!(trans & 0x1000)) {
                    if(trans == 0)
                        trans = 0x1001;
                    else
                        trans = 0x1000;
                }
            }
            else {
                trans &= ~0x1000;
            }
        }

        r += dr;

        /* Draw four cubes */
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, z);
        glRotatef(r, 0.0f, 1.0f, 0.5f);
        glPushMatrix();

        /* Always draw 2 cubes as solids */
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        glTranslatef(-5.0f, 0.0f, 0.0f);
        cube(r);

        glPopMatrix();
        glPushMatrix();
        glTranslatef(5.0f, 0.0f, 0.0f);
        cube(r);

        /* Potentially do two as translucent */
        if(trans & 1) {
            glEnable(GL_BLEND);
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
            glDisable(GL_CULL_FACE);
        }
        else
            glDisable(GL_BLEND);

        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.0f, 5.0f, 0.0f);
        cube(r);

        glPopMatrix();
        glTranslatef(0.0f, -5.0f, 0.0f);
        cube(r);

        if(trans & 1) {
            glEnable(GL_CULL_FACE);
        }

        /* Finish the frame */
        glKosSwapBuffers();
    }

    pvr_get_stats(&stats);
    printf("VBL Count: %d, last_time: %lld, frame rate: %f fps\n",
           stats.vbl_count, stats.frame_last_time, stats.frame_rate);

    return 0;
}


