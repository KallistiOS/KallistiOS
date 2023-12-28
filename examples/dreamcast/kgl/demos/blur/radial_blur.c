/* 
   KallistiOS 2.0.0

   radial_blur.c
   (c)2014 Josh Pearson
   (c)2001 Benoit Miller
   (c)2000 Tom Stanis/Jeff Molofee

   Radial Blur example, loosely based on nehe08.
*/

#include <kos.h>

#include <KGL/gl.h>
#include <KGL/glext.h>
#include <KGL/glu.h>
#include <KGL/glut.h>

/* 
   OpenGL example to demonstrate blending and lighting.
   Furthermore, this example demonstrates how to use render-to-texture.
   In this example, we use glutCopyBufferToTexture(...) to render
   submitted vertex data to a texture, without flushing the vertex
   data in main ram.  This makes using the "Radial Blur" effect
   quite efficient in terms of CPU usage.  GPU usage, however, is a
   different story altogether.  The PowerVR really struggles blending
   overlapped polygons, as we can see as we increase the number of
   polygons to be rendered into the translucent list by the radial
   blur effect.

   Other than that, essentially the same thing as NeHe's lesson08 code.
   To learn more, go to http://nehe.gamedev.net/.

   Radial blur effect was originally demonstrated here:
   http://nehe.gamedev.net/tutorial/radial_blur__rendering_to_a_texture/18004/

   DPAD controls the cube rotation, button A & B control the depth
   of the cube, button X enables radial blur, and button Y disables
   radial blur.  Left Trigger reduces number of times to render the radial blur
   effect, Right Trigger Increases.
*/

static GLfloat xrot;        /* X Rotation */
static GLfloat yrot;        /* Y Rotation */
static GLfloat xspeed;      /* X Rotation Speed */
static GLfloat yspeed;      /* Y Rotation Speed */
static GLfloat z = -5.0f;   /* Depth Into The Screen */

static GLuint      RENDER_TEXTURE_ID;      /* Render-To-Texture GL Texture ID */
static long unsigned int RENDER_TEXTURE_W; /* Render-To-Texture width */
static long unsigned int RENDER_TEXTURE_H; /* Render-To-Texture height */

static GLuint fbo[1];

extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

GLubyte InitRenderTexture(GLsizei width, GLsizei height) {
    RENDER_TEXTURE_W = width;
    RENDER_TEXTURE_H = height;

    /* Generate a texture for Open GL, and bind that texture */
    glGenTextures(1, &RENDER_TEXTURE_ID);
    glBindTexture(GL_TEXTURE_2D, RENDER_TEXTURE_ID);

    /* Submit an empty texture for storing the Frame Buffer Texture */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 RENDER_TEXTURE_W, RENDER_TEXTURE_H, 0,
                 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);

    /* Enable GL_LINEAR Texture Filter to enable bilinear filtering */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_LINEAR);

	/* Generate and Bind A Frame Buffer Object (FBO) For Open GL */
	glGenFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);

	/* Set the Generated texture as the storage for the FBO */
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           RENDER_TEXTURE_ID, 0);

	/* Verify the Frame Buffer Object is ready */
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return 0;
    }

	/* Un-bind the Frame Buffer Object, to restore the window-frame-buffer */
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 1;
}

void RenderBlurEffect(int times, float inc, long unsigned int width, long unsigned int height,
                      GLuint texID)

{
    float spost = 0.0f;                // Starting Texture Coordinate Offset
    float alphainc = 0.9f / times;     // Fade Speed For Alpha Blending
    float alpha = 0.2f;                // Starting Alpha Value
    float U, V;

    if(width > (float)vid_mode->width)
        U = (float)vid_mode->width / width;
    else
        U = 1;

    if(height > (float)vid_mode->height)
        V = (float)vid_mode->height / height;
    else
        V = 1;

    float W = (float)vid_mode->width;
    float H = (float)vid_mode->height;

    glDisable(GL_LIGHTING);                 // Disable GL Lighting
    glDisable(GL_DEPTH_TEST);               // Disable Depth Testing
    glEnable(GL_TEXTURE_2D);                // Enable 2D Texture Mapping
    glEnable(GL_BLEND);                     // Enable Blending

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);      // Set Blending Mode

    glBindTexture(GL_TEXTURE_2D, texID);  // Bind The Rendered Texture

    alphainc = alpha / times;                         // alphainc=0.2f / Times To Render Blur

    glBegin(GL_QUADS);

    while(times--) {                            // Number Of Times To Render Blur
        glColor4f(1.0f, 1.0f, 1.0f, alpha);     // Set The Alpha Value (Starts At 0.2)

        glTexCoord2f(0 + spost, 0 + spost);
        glKosVertex2f(0, 0);

        glTexCoord2f(U - spost, 0 + spost);
        glKosVertex2f(W, 0);

        glTexCoord2f(U - spost, V - spost);
        glKosVertex2f(W, H);

        glTexCoord2f(0 + spost, V - spost);
        glKosVertex2f(0, H);

        spost += inc;                   // Gradually Increase spost (Zooming Closer To Texture Center)

        alpha = alpha - alphainc;       // Gradually Decrease alpha (Gradually Fading Image Out)
    }

    glEnd();

    glDisable(GL_TEXTURE_2D);                   // Disable 2D Texture Mapping
    glDisable(GL_BLEND);                        // Disable Blending
    glEnable(GL_DEPTH_TEST);                    // Enable Depth Testing
    glEnable(GL_LIGHTING);                      // Enable Lighting
}

void DrawGL(GLuint texID) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, z);

    glRotatef(xrot, 1.0f, 0.0f, 0.0f);
    glRotatef(yrot, 0.0f, 1.0f, 0.0f);

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, texID);

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

    glDisable(GL_TEXTURE_2D);

    xrot += xspeed;
    yrot += yspeed;
}

#define ENABLE_RADIAL_BLUR 2
#define DISABLE_RADIAL_BLUR 3
#define INCREASE_RADIAL_BLUR 4
#define DECREASE_RADIAL_BLUR 5

int InputCallback(void) {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    /* Check key status */
    state = (cont_state_t *)maple_dev_status(cont);

    if(!state) {
        printf("Error reading controller\n");
        return -1;
    }

    if(state->buttons & CONT_START)
        return 0;

    if(state->buttons & CONT_A)
        z -= 0.02f;

    if(state->buttons & CONT_B)
        z += 0.02f;

    if((state->buttons & CONT_X)) {
        return ENABLE_RADIAL_BLUR;
    }

    if((state->buttons & CONT_Y)) {
        return DISABLE_RADIAL_BLUR;
    }

    if(state->buttons & CONT_DPAD_UP)
        xspeed -= 0.01f;

    if(state->buttons & CONT_DPAD_DOWN)
        xspeed += 0.01f;

    if(state->buttons & CONT_DPAD_LEFT)
        yspeed -= 0.01f;

    if(state->buttons & CONT_DPAD_RIGHT)
        yspeed += 0.01f;

    if(state->ltrig)
        return DECREASE_RADIAL_BLUR;

    if(state->rtrig)
        return INCREASE_RADIAL_BLUR;

    return 1;
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
    printf("glRadialBlur beginning\n");

    /* Get basic stuff initialized */
    glKosInit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, vid_mode->width / vid_mode->height, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* Enable Lighting and GL_LIGHT0 */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* Set up the textures */
    GLuint tex0 = glTextureLoadPVR("/rd/glass.pvr", 0, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_LINEAR);

    /* Set the Render Texture and Render-To-Texture Viewport Dimensions - Must be Power of two */
    InitRenderTexture(1024, 512);

    GLubyte enable_radial = 0, radial_iterations = 8;

    while(1) {

        if(check_start())
            break;

        /* Draw the GL "scene" */
        if(enable_radial) {
			/* Bind the Frame Buffer Object to GL, for render-to-texture */
			glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
             
			DrawGL(tex0);

            glutSwapBuffers(); 

			/* Un-Bind the Frame Buffer Object, for render-to-window */
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			DrawGL(tex0);

            RenderBlurEffect(radial_iterations, 0.02f, RENDER_TEXTURE_W, RENDER_TEXTURE_H, RENDER_TEXTURE_ID);

			glutSwapBuffers(); /* Finish the GL "scene" */
        }
		else{
		    DrawGL(tex0);

			glutSwapBuffers(); /* Finish the GL "scene" */
		}
		

        /* Very simple callback to handle user input based on static global vars */
        switch(InputCallback()) {

            case ENABLE_RADIAL_BLUR:
                enable_radial = 1;
                break;

            case DISABLE_RADIAL_BLUR:
                enable_radial = 0;
                break;

            case INCREASE_RADIAL_BLUR:
                if(radial_iterations < 18)
                    ++radial_iterations;

                printf("radial iterations: %i\n", radial_iterations);
                break;

            case DECREASE_RADIAL_BLUR:
                if(radial_iterations > 0)
                    --radial_iterations;

                printf("radial iterations: %i\n", radial_iterations);
                break;
        }
    }

    return 0;
}

