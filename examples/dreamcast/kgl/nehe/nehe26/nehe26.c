/* 
   KallistiOS 2.0.0

   nehe26.c
   (c)2014 Josh Pearson
   (c)2001 Benoit Miller
   (c)2000 Jeff Molofee
*/

#include <kos.h>

#include <stdlib.h>

#include <KGL/gl.h>
#include <KGL/glu.h>
#include <KGL/glut.h>

/* Morphing Points!

   Essentially the same thing as NeHe's lesson26 code.
   To learn more, go to http://nehe.gamedev.net/.

   Buttons A X Y B are used to choose the sphere, torus, tube, and random
   point cloud respectively.  DPAD and Right/Left triggers control object
   rotation.
*/

/* screen width, height, and bit depth */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 400

/* Set up some booleans */
#define TRUE  1
#define FALSE 0
typedef unsigned int bool;

/* Build Our Vertex Structure */
typedef struct {
    float x, y, z; /* 3D Coordinates */
} vertex;

/* Build Our Object Structure */
typedef struct {
    int verts;      /* Number Of Vertices For The Object */
    vertex *points; /* One Vertice (Vertex x,y & z)      */
} object;

GLfloat xrot, yrot, zrot;        /* Camera rotation variables                */
GLfloat cx, cy, cz = -15;        /* Camera pos variable                      */
GLfloat xspeed, yspeed, zspeed;  /* Spin Speed                               */

int key = 1;                     /* Make Sure Same Morph Key Not Pressed     */
int step = 0, steps = 200;       /* Step Counter And Maximum Number Of Steps */
bool morph = FALSE;              /* Default morph To False (Not Morphing)    */

int maxver;                            /* Holds The Max Number Of Vertices   */
object morph1, morph2, morph3, morph4, /* Our 4 Morphable Objects            */
       helper, *sour, *dest;          /* Helper, Source, Destination Object */

#define MORPHS 4

/* function to allocate memory for an object */
void objallocate(object *k, int n) {

    /* Sets points Equal To VERTEX * Number Of Vertices */
    k->points = (vertex *)malloc(sizeof(vertex) * n);
}

/* function deallocate memory for an object */
void objfree(object *k) {

    free(k->points);
}


/* function to release/destroy our resources and restoring the old desktop */
void Quit(int returnCode) {

    /* deallocate the objects' memory */
    objfree(&morph1);
    objfree(&morph2);
    objfree(&morph3);
    objfree(&morph4);
    objfree(&helper);
}

/* function Loads Object From File (name) */
void objload(char *name, object *k) {

    int ver;           /* Will Hold Vertice Count                */
    float rx, ry, rz;  /* Hold Vertex X, Y & Z Position          */
    FILE *filein;      /* Filename To Open                       */
    int i;             /* Simple loop variable                   */

    printf("  [objload] file: %s\n", name);

    /* Opens The File For Reading */
    filein = fopen(name, "r");
    /* Reads the number of verts in the file */
    fread(&ver, sizeof(int), 1, filein);
    /* Sets Objects verts Variable To Equal The Value Of ver */
    k->verts = ver;
    /* Jumps To Code That Allocates Ram To Hold The Object */
    objallocate(k, ver);

    /* Loops Through The Vertices */
    for(i = 0; i < ver; i++) {
        /* Reads the next three verts */
        fread(&rx, sizeof(float), 1, filein);
        fread(&ry, sizeof(float), 1, filein);
        fread(&rz, sizeof(float), 1, filein);
        /* Set our object's x, y, z points */
        k->points[i].x = rx;
        k->points[i].y = ry;
        k->points[i].z = rz;
    }

    /* Close The File */
    fclose(filein);

    /* If ver Is Greater Than maxver Set maxver Equal To ver */
    if(ver > maxver)
        maxver = ver;
}

/* function to calculate Movement Of Points During Morphing */
vertex calculate(int i) {

    vertex a; /* Temporary Vertex Called a */

    /* Calculate x, y, and z movement */
    a.x = (sour->points[i].x - dest->points[i].x) / steps;
    a.y = (sour->points[i].y - dest->points[i].y) / steps;
    a.z = (sour->points[i].z - dest->points[i].z) / steps;

    return a;
}

/* general OpenGL initialization function */
int initGL(GLvoid) {

    int i; /* Simple Looping variable */

    /* Height / width ration */
    GLfloat ratio;

    ratio =  SCREEN_WIDTH / SCREEN_HEIGHT;

    /* change to the projection matrix and set our viewing volume. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* Set our perspective */
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);

    /* Make sure we're changing the model view and not the projection */
    glMatrixMode(GL_MODELVIEW);

    /* Reset The View */
    glLoadIdentity();

    /* Set The Blending Function For Translucency */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    /* This Will Clear The Background Color To Black */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    /* Enables Clearing Of The Depth Buffer */
    glClearDepth(1.0);

    /* The Type Of Depth Test To Do */
    glDepthFunc(GL_LESS);

    /* Enables Depth Testing */
    glEnable(GL_DEPTH_TEST);

    /* Enables Smooth Color Shading */
    glShadeModel(GL_SMOOTH);

    /* Sets Max Vertices To 0 By Default */
    maxver = 0;
    /* Load The First Object Into morph1 From File sphere.txt */
    objload("/rd/sphere.bin", &morph1);
    /* Load The Second Object Into morph2 From File torus.txt */
    objload("/rd/torus.bin", &morph2);
    /* Load The Third Object Into morph3 From File tube.txt */
    objload("/rd/tube.bin", &morph3);

    /* Manually Reserve Ram For A 4th 486 Vertice Object (morph4) */
    objallocate(&morph4, 486);

    /* Loop Through All 468 Vertices */
    for(i = 0; i < 486; i++) {
        /* Generate a random point in xyz space for each vertex */
        /* Values range from -7 to 7                            */
        morph4.points[i].x = ((float)(rand() % 14000) / 1000) - 7;
        morph4.points[i].y = ((float)(rand() % 14000) / 1000) - 7;
        morph4.points[i].z = ((float)(rand() % 14000) / 1000) - 7;
    }

    /* Load sphere.txt Object Into Helper (Used As Starting Point) */
    objload("/rd/sphere.bin", &helper);
    /* Source & Destination Are Set To Equal First Object (morph1) */
    sour = dest = &morph1;

    return(TRUE);
}

void draw_gl(void) {

    GLfloat tx, ty, tz; /* Temp X, Y & Z Variables                     */
    vertex q;       /* Holds Returned Calculated Values For One Vertex */
    int i;          /* Simple Looping Variable                         */

    /* Clear The Screen And The Depth Buffer */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Reset The View */
    glLoadIdentity();
    /* Translate The The Current Position To Start Drawing */
    glTranslatef(cx, cy, cz);
    /* Rotate On The X Axis By xrot */
    glRotatef(xrot, 1.0f, 0.0f, 0.0f);
    /* Rotate On The Y Axis By yrot */
    glRotatef(yrot, 0.0f, 1.0f, 0.0f);
    /* Rotate On The Z Axis By zrot */
    glRotatef(zrot, 0.0f, 0.0f, 1.0f);

    /* Increase xrot,yrot & zrot by xspeed, yspeed & zspeed */
    xrot += xspeed;
    yrot += yspeed;
    zrot += zspeed;

    /* Begin Drawing Points */
    glBegin(GL_POINTS);

    /* Loop Through All The Verts Of morph1 (All Objects Have
     * * The Same Amount Of Verts For Simplicity, Could Use maxver Also)
     * */
    for(i = 0; i < morph1.verts; i++) {
        /* If morph Is True Calculate Movement Otherwise Movement=0 */
        if(morph)
            q = calculate(i);
        else
            q.x = q.y  = q.z = 0.0f;

        /* Subtract the calculated unitx from the point's xyz coords */
        helper.points[i].x -= q.x;
        helper.points[i].y -= q.y;
        helper.points[i].z -= q.z;

        /* Set the temp xyz vars the the helper's xyz vars */
        tx = helper.points[i].x;
        ty = helper.points[i].y;
        tz = helper.points[i].z;

        /* Set Color To A Bright Shade Of Off Blue */
        glColor3f(0.0f, 1.0f, 1.0f);
        /* Draw A Point At The Current Temp Values (Vertex) */
        glVertex3f(tx, ty, tz);
        /* Darken Color A Bit */
        glColor3f(0.0f, 0.5f, 1.0f);
        /* Calculate Two Positions Ahead */
        tx -= 2.0f * q.x;
        ty -= 2.0f * q.y;
        tz -= 2.0f * q.z;

        /* Draw A Second Point At The Newly Calculate Position */
        glVertex3f(tx, ty, tz);
        /* Set Color To A Very Dark Blue */
        glColor3f(0.0f, 0.0f, 1.0f);

        /* Calculate Two More Positions Ahead */
        tx -= 2.0f * q.x;
        ty -= 2.0f * q.y;
        tz -= 2.0f * q.z;

        /* Draw A Third Point At The Second New Position */
        glVertex3f(tx, ty, tz);
    }

    glEnd();

    /* If We're Morphing And We Haven't Gone Through All 200 Steps
     * Increase Our Step Counter
     * Otherwise Set Morphing To False, Make Source=Destination And
     * Set The Step Counter Back To Zero.
     */

    if(morph && step <= steps) {
        step++;
    }
    else {
        morph = FALSE;
        sour  = dest;
        step  = 0;
    }
}

#define NOT_LAST !(state->buttons & last)

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;
    uint16  last = CONT_A;

    xrot = yrot = zrot = 0.0f;
    xspeed = yspeed = zspeed = 0.0f;

    printf("nehe26 beginning\n");

    /* Get basic stuff initialized */
    glKosInit();
    initGL();

    printf("Entering main loop\n");

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

        if((state->buttons & CONT_A) && !morph && NOT_LAST) {
            morph = TRUE;
            dest = &morph1;
            last = CONT_A;
        }

        if((state->buttons & CONT_X) && !morph && NOT_LAST) {
            morph = TRUE;
            dest = &morph2;
            last = CONT_X;
        }

        if((state->buttons & CONT_Y) && !morph && NOT_LAST) {
            morph = TRUE;
            dest = &morph3;
            last = CONT_Y;
        }

        if((state->buttons & CONT_B) && !morph && NOT_LAST) {
            morph = TRUE;
            dest = &morph4;
            last = CONT_B;
        }

        if(state->buttons & CONT_DPAD_UP)
            xspeed -= 0.01f;

        if(state->buttons & CONT_DPAD_DOWN)
            xspeed += 0.01f;

        if(state->buttons & CONT_DPAD_LEFT)
            yspeed -= 0.01f;

        if(state->buttons & CONT_DPAD_RIGHT)
            yspeed += 0.01f;

        if(state->rtrig > 0x7f)
            zspeed += 0.01f;

        if(state->ltrig > 0x7f)
            zspeed -= 0.01f;

        draw_gl();

        /* Finish the frame */
        glutSwapBuffers();
    }

    Quit(0);
    return 0;
}

