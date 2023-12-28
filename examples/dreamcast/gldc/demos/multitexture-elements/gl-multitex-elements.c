/* 
   KallistiOS 2.0.0

   gl-multitex-elements.c
   (c)2014 Josh Pearson

   Open GL Multi-Texture example using Vertex Array Submission.
*/

#include <kos.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#include <GL/glkos.h>

/* Load a PVR texture - located in pvr-texture.c */
extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

GLfloat VERTEX_ARRAY[4 * 3] = { -1.0f,  1.0f, 0.0f,
                                1.0f,  1.0f, 0.0f,
                                1.0f, -1.0f, 0.0f,
                                -1.0f, -1.0f, 0.0f
                              };

GLfloat TEXCOORD_ARRAY[4 * 2] = { 0, 0,
                                  1, 0,
                                  1, 1,
                                  0, 1
                                };

GLuint ARGB_ARRAY[4 * 1] = { 0xFFFF0000, 0xFF0000FF, 0xFF00FF00, 0xFFFFFF00 };

GLubyte INDEX_ARRAY[4 * 1] = { 0, 1, 3, 2 };

/* Multi-Texture Example using Open GL Vertex Buffer Submission. */
void RenderCallback(GLuint texID0, GLuint texID1) {
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);
    
    /* Enable Client States for OpenGL Arrays Submission */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    
    /* Bind texture to GL_TEXTURE0_ARB and set texture parameters */
    glActiveTextureARB(GL_TEXTURE0_ARB); 
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    /* Bind multi-texture to GL_TEXTURE1_ARB and set texture parameters */
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    /* Set Blending Mode */
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
    
    /* Bind texture coordinates to GL_TEXTURE0_ARB */
    glTexCoordPointer(2, GL_FLOAT, 0, TEXCOORD_ARRAY);
    
    /* Bind texture coordinates to GL_TEXTURE1_ARB */
    glClientActiveTextureARB(GL_TEXTURE1_ARB); 
    glTexCoordPointer(2, GL_FLOAT, 0, TEXCOORD_ARRAY);  
    glClientActiveTextureARB(GL_TEXTURE0_ARB); 
  
    /* Bind the Color Array */
    glColorPointer(1, GL_UNSIGNED_INT, 0, ARGB_ARRAY);

    /* Bind the Vertex Array */
    glVertexPointer(3, GL_FLOAT, 0, VERTEX_ARRAY);
    
    /* Render the Vertices as Indexed Arrays using glDrawElements */
    glDrawElements(GL_TRIANGLE_STRIP, 4 * 1, GL_UNSIGNED_BYTE, INDEX_ARRAY);
    
    /* Disable GL_TEXTURE1 */
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
      
    /* Make sure to set glActiveTexture back to GL_TEXTURE0_ARB when finished */
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
 
    /* Disable Vertex, Color and Texture Coord Arrays */
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
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

    /* Load two PVR textures to OpenGL */
    GLuint texID0 = glTextureLoadPVR("/rd/wp001vq.pvr", 0, 0);
    GLuint texID1 = glTextureLoadPVR("/rd/FlareWS_256.pvr", 0, 0);

    while(1) {
        if(check_start())
            break;
            
        /* Draw the "scene" */
        RenderCallback(texID0, texID1);

        /* Finish the frame */
        glKosSwapBuffers();
    }

    return 0;
}

