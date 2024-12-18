
#ifndef __VQ_INTERNAL_H
#define __VQ_INTERNAL_H

#include "vq_types.h"

static void inline get_color(fcolor_t *c, uint8 *pixels) {
    c->a = pixels[0];
    c->r = pixels[1];
    c->g = pixels[2];
    c->b = pixels[3];
}

static void inline sum_colors(fcolor_t *out, fcolor_t *in) {
    out->a += in->a;
    out->r += in->r;
    out->g += in->g;
    out->b += in->b;
}

static void inline div_colors(fcolor_t *out, float v) {
    out->a /= v;
    out->r /= v;
    out->g /= v;
    out->b /= v;
}

static void inline clear_quad(fquad_t *q) {
    memset(q, '\0', sizeof(*q));
}

static void inline add_quad(fquad_t *out, fquad_t *in) {
    sum_colors(&out->p[0], &in->p[0]);
    sum_colors(&out->p[1], &in->p[1]);
    sum_colors(&out->p[2], &in->p[2]);
    sum_colors(&out->p[3], &in->p[3]);
}

static void inline sub_quad(fquad_t *out, fquad_t *a, fquad_t *b) {
    int i;

    for(i = 0; i < 4; i++) {
        out->p[i].a = a->p[i].a - b->p[i].a;
        out->p[i].r = a->p[i].r - b->p[i].r;
        out->p[i].g = a->p[i].g - b->p[i].g;
        out->p[i].b = a->p[i].b - b->p[i].b;
    }
}

static void inline div_quad(fquad_t *q, float v) {
    if(v < 1.0f) {
        clear_quad(q);
    }
    else {
        div_colors(&q->p[0], v);
        div_colors(&q->p[1], v);
        div_colors(&q->p[2], v);
        div_colors(&q->p[3], v);
    }
}

static void inline copy_quad(fquad_t *out, fquad_t *in) {
    *out = *in;
}

#endif  /* __VQ_INTERNAL_H */
