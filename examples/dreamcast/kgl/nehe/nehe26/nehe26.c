/* 
   KallistiOS 2.0.0

   nehe26.c
   Copyright (c) 2014 Josh Pearson
   Copyright (c) 2001 Benoit Miller
   Copyright (c) 2000 Jeff Molofee
*/

#include <kos.h>

#include <stdlib.h>
#include <stdbool.h>

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

/* Screen width, height, and bit depth */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 400

/* Build our vertex structure. */
typedef struct {
    float x, y, z; /* 3D coordinates */
} vertex;

/* Build our object structure. */
typedef struct {
    int verts;      /* Number of vertices for the object */
    vertex *points; /* One Vertex (Vertex x,y & z)      */
} object;

GLfloat xrot, yrot, zrot;        /* Camera rotation variables                */
GLfloat cx, cy, cz = -15;        /* Camera pos variable                      */
GLfloat xspeed, yspeed, zspeed;  /* Spin speed                               */

int key = 1;                     /* Make sure same morph key not pressed     */
int step = 0, steps = 200;       /* Step counter and maximum number of steps */
bool morph = false;              /* Default morph to false (not morphing)    */

int maxver;                            /* Holds the max number of vertices   */
object morph1, morph2, morph3, morph4, /* Our 4 morphable objects            */
       helper, *sour, *dest;           /* Helper, source, destination object */

#define MORPHS 4

/* Function to allocate memory for an object */
void objallocate(object *k, int n) {

    /* Sets points equal to vertex * number of vertices */
    k->points = (vertex *)malloc(sizeof(vertex) * n);
}

/* Function deallocate memory for an object */
void objfree(object *k) {

    free(k->points);
}


/* Function to release/destroy our resources and restoring the old desktop */
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

    int ver;           /* Will hold vertex count                */
    float rx, ry, rz;  /* Hold vertex X, Y & Z position          */
    FILE *filein;      /* Filename to open                       */
    int i;             /* Simple loop variable                   */

    printf("  [objload] file: %s\n", name);

    /* Opens the file for reading. */
    filein = fopen(name, "r");
    /* Reads the number of verts in the file. */
    fread(&ver, sizeof(int), 1, filein);
    /* Sets object's verts variable to equal the value of ver. */
    k->verts = ver;
    /* Jumps to code that allocates RAM to hold the object. */
    objallocate(k, ver);

    /* Loops through the vertices */
    for(i = 0; i < ver; i++) {
        /* Reads the next three verts */
        fread(&rx, sizeof(float), 1, filein);
        fread(&ry, sizeof(float), 1, filein);
        fread(&rz, sizeof(float), 1, filein);
        /* Set our object's x, y, z points. */
        k->points[i].x = rx;
        k->points[i].y = ry;
        k->points[i].z = rz;
    }

    /* Close the file. */
    fclose(filein);

    /* If ver is greater than maxver, set maxver equal to ver. */
    if(ver > maxver)
        maxver = ver;
}

/* Function to calculate movement of points during morphing */
vertex calculate(int i) {

    vertex a; /* Temporary vertex called 'a' */

    /* Calculate x, y, and z movement */
    a.x = (sour->points[i].x - dest->points[i].x) / steps;
    a.y = (sour->points[i].y - dest->points[i].y) / steps;
    a.z = (sour->points[i].z - dest->points[i].z) / steps;

    return a;
}

/* General OpenGL-initialization function */
int initGL(GLvoid) {

    int i; /* Simple Looping variable */

    /* Height / width ration */
    GLfloat ratio;

    ratio =  SCREEN_WIDTH / SCREEN_HEIGHT;

    /* Change to the projection matrix and set our viewing volume. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* Set our perspective.*/
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);

    /* Make sure we're changing the model view and not the projection. */
    glMatrixMode(GL_MODELVIEW);

    /* Reset the view. */
    glLoadIdentity();

    /* Set the blending function for translucency. */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    /* This will clear the background color to black. */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    /* Enables clearing of the depth buffer. */
    glClearDepth(1.0);

    /* The type of depth test to do. */
    glDepthFunc(GL_LESS);

    /* Enables depth testing. */
    glEnable(GL_DEPTH_TEST);

    /* Enables smooth color shading. */
    glShadeModel(GL_SMOOTH);

    /* Sets max vertices to 0 by default. */
    maxver = 0;
    /* Load the first object into morph1 from file sphere.txt. */
    objload("/rd/sphere.bin", &morph1);
    /* Load the second object into morph2 from file torus.txt. */
    objload("/rd/torus.bin", &morph2);
    /* Load the third pobject into morph3 from file tube.txt. */
    objload("/rd/tube.bin", &morph3);

    /* Manually reserve RAM for a 4th 486 vertice object (morph4). */
    objallocate(&morph4, 486);

    /* Loop through all 468 vertices */
    for(i = 0; i < 486; i++) {
        /* Generate a random point in xyz space for each vertex */
        /* Values range from -7 to 7.                           */
        morph4.points[i].x = ((float)(rand() % 14000) / 1000) - 7;
        morph4.points[i].y = ((float)(rand() % 14000) / 1000) - 7;
        morph4.points[i].z = ((float)(rand() % 14000) / 1000) - 7;
    }

    /* Load sphere.txt Object Into Helper (Used As Starting Point) */
    objload("/rd/sphere.bin", &helper);
    /* Source & Destination Are Set To Equal First Object (morph1) */
    sour = dest = &morph1;

    return(true);
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

    /* Increase xrot, yrot & zrot by xspeed, yspeed & zspeed. */
    xrot += xspeed;
    yrot += yspeed;
    zrot += zspeed;

    /* Begin drawing points. */
    glBegin(GL_POINTS);

    /* Loop through all the verts of morph1 (all objects have
     * * the same amount of verts for simplicity, could use maxver also).
     * */
    for(i = 0; i < morph1.verts; i++) {
        /* If morph is true calculate movement, otherwise movement=0 */
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

        /* Set color to a bright shade of off-blue. */
        glColor3f(0.0f, 1.0f, 1.0f);
        /* Draw a point at the current temp values (vertex). */
        glVertex3f(tx, ty, tz);
        /* Darken Color A Bit */
        glColor3f(0.0f, 0.5f, 1.0f);
        /* Calculate two positions ahead. */
        tx -= 2.0f * q.x;
        ty -= 2.0f * q.y;
        tz -= 2.0f * q.z;

        /* Draw a second point at the newly calculated position. */
        glVertex3f(tx, ty, tz);
        /* Set color to a very dark blue. */
        glColor3f(0.0f, 0.0f, 1.0f);

        /* Calculate two more positions ahead. */
        tx -= 2.0f * q.x;
        ty -= 2.0f * q.y;
        tz -= 2.0f * q.z;

        /* Draw a third point at the second new position. */
        glVertex3f(tx, ty, tz);
    }

    glEnd();

    /* If we're morphing, and we haven't gone through all 200 steps,
     * increase our step counter.
     * Otherwise, set morphing to false, make source=destination, and
     * set the step counter back to zero.
     */

    if(morph && step <= steps) {
        step++;
    }
    else {
        morph = false;
        sour  = dest;
        step  = 0;
    }
}

#define NOT_LAST !(state->buttons & last)

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;
    uint16_t  last = CONT_A;

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
            morph = true;
            dest = &morph1;
            last = CONT_A;
        }

        if((state->buttons & CONT_X) && !morph && NOT_LAST) {
            morph = true;
            dest = &morph2;
            last = CONT_X;
        }

        if((state->buttons & CONT_Y) && !morph && NOT_LAST) {
            morph = true;
            dest = &morph3;
            last = CONT_Y;
        }

        if((state->buttons & CONT_B) && !morph && NOT_LAST) {
            morph = true;
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

