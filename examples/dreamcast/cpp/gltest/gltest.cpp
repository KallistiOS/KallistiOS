/* KallistiOS ##version##

   gltest.cpp
   (c)2014 Josh Pearson
   (c)2001-2002 Megan Potter
*/

#include <kos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/glkos.h>

/*

This is a really simple KallistiGL example. It shows off several things:
basic matrix control, perspective, and controlling the image with maple input.

Thanks to NeHe's tutorials for the crate image.

This version is written in C++ and uses some basic C++ operations to test
various things about C++ functionality.

*/

class Object {
protected:
    float   tx, ty, tz;

public:
    Object(float dtx, float dty, float dtz) {
        tx = dtx;
        ty = dty;
        tz = dtz;
        printf("Object::Object called\n");
    }

    virtual ~Object() {
        printf("Object::~Object called\n");
    }

    virtual void draw() {
    }
};

class Cube : public Object {
private:
    float r;

public:
    Cube(float px, float py, float pz)
        : Object(px, py, pz) {
        r = 0.0f;
        printf("Cube::Cube called\n");
    }

    virtual ~Cube() {
        printf("Cube::~Cube called\n");
    }

    void rotate(float dr) {
        r += dr;
    }

    /* Draw a cube centered around 0,0,0. */
    virtual void draw() {
        glPushMatrix();
        glTranslatef(tx, ty, tz);
        glRotatef(r, 1.0f, 0.0f, 1.0f);

		float uv[4][2] = { { 0.0f, 0.0f },
			               { 1.0f, 0.0f },
						   { 1.0f, 1.0f },
						   { 0.0f, 1.0f } };

        glBegin(GL_QUADS);

        /* Front face */
		glTexCoord2fv(&uv[0][0]);
        glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2fv(&uv[1][0]);
        glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2fv(&uv[2][0]);
        glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2fv(&uv[3][0]);
        glVertex3f(-1.0f, -1.0f, 1.0f);

        /* Back face */
		glTexCoord2fv(&uv[0][0]);
        glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2fv(&uv[1][0]);
        glVertex3f(1.0f, -1.0f, -1.0f);
		glTexCoord2fv(&uv[2][0]);
        glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2fv(&uv[3][0]);
        glVertex3f(-1.0f, 1.0f, -1.0f);

        /* Left face */
		glTexCoord2fv(&uv[0][0]);
        glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2fv(&uv[1][0]);
        glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2fv(&uv[2][0]);
        glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2fv(&uv[3][0]);
        glVertex3f(-1.0f, 1.0f, -1.0f);

        /* Right face */
		glTexCoord2fv(&uv[0][0]);
        glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2fv(&uv[1][0]);
        glVertex3f(1.0f, -1.0f, -1.0f);
		glTexCoord2fv(&uv[2][0]);
        glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2fv(&uv[3][0]);
        glVertex3f(1.0f, 1.0f, 1.0f);

        /* Top face */
		glTexCoord2fv(&uv[0][0]);
        glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2fv(&uv[1][0]);
        glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2fv(&uv[2][0]);
        glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2fv(&uv[3][0]);
        glVertex3f(1.0f, 1.0f, -1.0f);

        /* Bottom face */
		glTexCoord2fv(&uv[0][0]);
        glVertex3f(1.0f, -1.0f, -1.0f);
		glTexCoord2fv(&uv[1][0]);
        glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2fv(&uv[2][0]);
        glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2fv(&uv[3][0]);
        glVertex3f(1.0f, -1.0f, 1.0f);

        glEnd();

        glPopMatrix();
    }
};

/* Note that ctors/dtors aren't really supported right now because of the
   problems related to initialization; i.e., when main() is called, ctors
   ought to have been called, but if they do any KOS specific stuff, it
   will work (or worse, crash). I'm open to suggestions on fixing it. */
class CtorDtorTest {
public:
    CtorDtorTest() {
        printf("CtorDtorTest::CtorDtorTest called\n");
    }
    virtual ~CtorDtorTest() {
        printf("CtorDtorTest::~CtorDtorTest called\n");
    }
} test_object, test_object2;

/* Load a PVR texture using glTexImage2D */
void loadtxr(const char *fname, GLuint *txr) {
#define PVR_HDR_SIZE 0x20
    FILE *tex = NULL;
    unsigned char *texBuf;
    unsigned int texSize;

    tex = fopen(fname, "rb");

    if(tex == NULL) {
        printf("FILE READ ERROR: %s\n", fname);

        while(1);
    }

    fseek(tex, 0, SEEK_END);
    texSize = ftell(tex);

    texBuf = (unsigned char*)malloc(texSize);
    fseek(tex, 0, SEEK_SET);
    fread(texBuf, 1, texSize, tex);
    fclose(tex);

    int texW = texBuf[PVR_HDR_SIZE - 4] | texBuf[PVR_HDR_SIZE - 3] << 8;
    int texH = texBuf[PVR_HDR_SIZE - 2] | texBuf[PVR_HDR_SIZE - 1] << 8;
    int texFormat, texColor;

    switch((unsigned int)texBuf[PVR_HDR_SIZE - 8]) {
        case 0x00:
            texColor = PVR_TXRFMT_ARGB1555;
            break; //(bilevel translucent alpha 0,255)

        case 0x01:
            texColor = PVR_TXRFMT_RGB565;
            break; //(non translucent RGB565 )

        case 0x02:
            texColor = PVR_TXRFMT_ARGB4444;
            break; //(translucent alpha 0-255)

        case 0x03:
            texColor = PVR_TXRFMT_YUV422;
            break; //(non translucent UYVY )

        case 0x04:
            texColor = PVR_TXRFMT_BUMP;
            break; //(special bump-mapping format)

        case 0x05:
            texColor = PVR_TXRFMT_PAL4BPP;
            break; //(4-bit palleted texture)

        case 0x06:
            texColor = PVR_TXRFMT_PAL8BPP;
            break; //(8-bit palleted texture)

        default:
			texColor = PVR_TXRFMT_RGB565;
            break;
    }

    switch((unsigned int)texBuf[PVR_HDR_SIZE - 7]) {
        case 0x01:
            texFormat = PVR_TXRFMT_TWIDDLED;
            break;//SQUARE TWIDDLED

        case 0x03:
            texFormat = PVR_TXRFMT_VQ_ENABLE;
            break;//VQ TWIDDLED

        case 0x09:
            texFormat = PVR_TXRFMT_NONTWIDDLED;
            break;//RECTANGLE

        case 0x0B:
            texFormat = PVR_TXRFMT_STRIDE | PVR_TXRFMT_NONTWIDDLED;
            break;//RECTANGULAR STRIDE

        case 0x0D:
            texFormat = PVR_TXRFMT_TWIDDLED;
            break;//RECTANGULAR TWIDDLED

        case 0x10:
            texFormat = PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED;
            break;//SMALL VQ

        default:
            texFormat = PVR_TXRFMT_NONE;
            break;
    }

    printf("TEXTURE Resolution: %ix%i\n", texW, texH);

    glGenTextures(1, txr);
    glBindTexture(GL_TEXTURE_2D, *txr);

    if(texFormat & PVR_TXRFMT_VQ_ENABLE)
        glCompressedTexImage2D(GL_TEXTURE_2D,
                               0,
 	                       texFormat | texColor,
 	                       texW,
 	                       texH,
 	                       0,
 	                       texSize,
 	                       texBuf);
    else
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     texW, texH,
                     0,
                     GL_RGB,
                     texFormat | texColor,
                     texBuf);
}

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;
    float   r = 0.0f;
    float   dr = 2.0f;
    float   z = -14.0f;
    GLuint  texture;
    int trans = 0;

    /* Initialize KOS */
    dbglog_set_level(DBG_WARNING);

    printf("gltest beginning\n");

    /* Get basic stuff initialized */
    glKosInit();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, vid_mode->width / (GLfloat)vid_mode->height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);

    /* Expect CW vertex order */
    glFrontFace(GL_CW);

    /* Enable Transparency */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Load a texture and make it look nice */
    loadtxr("/rd/glass.pvr", &texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    printf("texture is %08x\n", texture);

    Cube *cubes[4] = {
        new Cube(-5.0f, 0.0f, 0.0f),
        new Cube(5.0f, 0.0f, 0.0f),
        new Cube(0.0f, 5.0f, 0.0f),
        new Cube(0.0f, -5.0f, 0.0f)
    };
    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    while(1) {
        /* Check key status */
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state) {
            printf("Error reading controller\n");
            break;
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

            for(int i = 0; i < 4; i++)
                cubes[i]->rotate(- 2.0f);

            r -= 2.0f;
        }

        if(state->buttons & CONT_DPAD_RIGHT) {
            dr = 0.0f;

            for(int i = 0; i < 4; i++)
                cubes[i]->rotate(+ 2.0f);

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

        for(int i = 0; i < 4; i++)
            cubes[i]->rotate(dr);

        r += dr;

        /* Draw four objects */
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, z);
        glRotatef(r, 0.0f, 1.0f, 0.5f);

	/* Always draw 2 cubes as solids */
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);        
	
	cubes[0]->draw();
        cubes[1]->draw();
        /* Potentially do two as translucent */
        if(trans & 1) {
            glEnable(GL_BLEND);
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
            glDisable(GL_CULL_FACE);
        }

        cubes[2]->draw();
        cubes[3]->draw();

        /* Finish the frame */
        glKosSwapBuffers();
    }

    for(int i = 0; i < 4; i++) {
        delete cubes[i];
    }

	glDeleteTextures(1, &texture);

    return 0;
}


