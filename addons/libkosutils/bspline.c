/* KallistiOS ##version##

   bspline.c
   (c)2000 Megan Potter

   For generating B-Spline curves

   This math info is from Leendert Ammeraal's "Programming Principles in
   Computer Graphics".
*/

#include <kos/bspline.h>

/* Pass it an array of points and it will calculate a set of B-spline
   co-efficients for generating a curve. There must be at least one point
   before the "current" one, and at least two after the "current" one (a
   total of four points required). These values will be used in the
   function below. */
static float bc_x[4], bc_y[4], bc_z[4];
void bspline_coeff(const point_t * pnt) {
    float a, b, c, d;

    /* First calculate the X coefficients */
    a = pnt[-1].x;
    b = pnt[0].x;
    c = pnt[1].x;
    d = pnt[2].x;
    bc_x[3] = (-a + 3 * (b - c) + d) / 6.0f;
    bc_x[2] = (a - 2 * b + c) / 2.0f;
    bc_x[1] = (c - a) / 2.0f;
    bc_x[0] = (a + 4 * b + c) / 6.0f;

    /* Next, the Y coefficients */
    a = pnt[-1].y;
    b = pnt[0].y;
    c = pnt[1].y;
    d = pnt[2].y;
    bc_y[3] = (-a + 3 * (b - c) + d) / 6.0f;
    bc_y[2] = (a - 2 * b + c) / 2.0f;
    bc_y[1] = (c - a) / 2.0f;
    bc_y[0] = (a + 4 * b + c) / 6.0f;

    /* Finally, the Z coefficients */
    a = pnt[-1].z;
    b = pnt[0].z;
    c = pnt[1].z;
    d = pnt[2].z;
    bc_z[3] = (-a + 3 * (b - c) + d) / 6.0f;
    bc_z[2] = (a - 2 * b + c) / 2.0f;
    bc_z[1] = (c - a) / 2.0f;
    bc_z[0] = (a + 4 * b + c) / 6.0f;
}

/* Given a 't' (between 0.0f and 1.0f) this will generate the next point
   values for the current set of coefficients. */
void bspline_get_point(float t, point_t *p) {
    /* Generate X */
    p->x = ((bc_x[3] * t + bc_x[2]) * t + bc_x[1]) * t + bc_x[0];

    /* Generate Y */
    p->y = ((bc_y[3] * t + bc_y[2]) * t + bc_y[1]) * t + bc_y[0];

    /* Generate Z */
    p->z = ((bc_z[3] * t + bc_z[2]) * t + bc_z[1]) * t + bc_z[0];
}






