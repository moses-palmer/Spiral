#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <para/para.h>

#include "spiral.h"

/**
 * The width of the anti aliased border around a spiral line.
 */
#define ANTI_ALIAS_BORDER 0.08

/**
 * The minimum diameter of the centre circle.
 */
#define CENTER_RADIUS 3.0

struct Spiral {
    /** The spiral data, which is an array of bytes; the size of the buffer is
        height * width */
    unsigned char *data;

    /** The dimensions of the buffer */
    unsigned int width, height;

    /** The number of curves that extend from the centre **/
    unsigned int curves;

     /** The number of alterations of direction of the curves */
    unsigned int alterations;

    /** The radius of the spiral; pixels further from the centre will be
        black */
    unsigned int radius;

    /** The twist to apply */
    double twist;

    /** The width of the curves; 0.5 means that half of the spiral will be
        painted with the foreground colour and half with the background
        colour */
    double line_width;
};

/**
 * Calculates the distance to the centre of a line.
 *
 * @param s
 *     The spiral.
 * @param h, angle
 *     The polar coordinates of the point.
 * @return the distance to the centre of a line
 */
static inline double
get_distance_to_line(Spiral *s, double h, double angle)
{
    double segment, t, twisted;

    /* Calculate the current segment, and how far we have reached within it */
    t = modf(s->alterations * h / s->radius, &segment);

    /* Calculate the twisted angle */
    twisted = fmod(s->curves * (angle + ((int)segment % 2
        ? t * s->twist
        : s->twist * (1.0 - t))), 2 * M_PI);
    if (twisted < 0.0) {
        twisted += 2 * M_PI;
    }

    /* Return the absolute value of the distance to 0.5 */
    return 2 * fabs((twisted / (2 * M_PI) - 0.5));
}

static int
spiral_initialize_do(Spiral *s, int start, int end, int gstart, int gend)
{
    int x, y;
    unsigned char *d;
    double cx = 0.5 * s->width;
    double cy = 0.5 * s->height;
    int center_radius = (int)sqrt(s->curves * CENTER_RADIUS);

    for (y = start; y < end; y++) {
        double dy = y - cy;

        /* Point d to the current scan line */
        d = s->data + y * s->width;
        for (x = 0; x < s->width; x++) {
            double dx = x - cx;
            double h = hypot(dx, dy);
            unsigned int alpha = 0;

            /* Include one extra pixel to enable anti aliasing */
            if (h < s->radius + 1) {
                double angle = atan2(dy, dx);
                double distance;

                distance = get_distance_to_line(s, h, angle);

                /* Include the anti alias border to allow a smooth border of the
                   spiral line */
                if (distance < s->line_width + ANTI_ALIAS_BORDER) {
                    if (distance > s->line_width) {
                        alpha = (unsigned int)(255
                            - 255 * (distance - s->line_width)
                                / ANTI_ALIAS_BORDER);
                    }
                    else {
                        alpha = 255;
                    }
                }
                else {
                    alpha = 0;
                }

                if ((int)h < center_radius) {
                    alpha = 255;
                }
                else if ((int)h == center_radius) {
                    double a = h - center_radius;
                    alpha = (unsigned int)(alpha * a + 255 * (1.0 - a));
                }

                /* If h is equal to or a fraction grater than the radius, we
                   decrease the alpha */
                if ((int)h == (int)s->radius) {
                    alpha = (unsigned int)(alpha * (h - s->radius));
                }
            }
            else {
                alpha = 0;
            }

            *d = alpha;
            d++;
        }
    }

    return 0;
}

Spiral*
spiral_create(unsigned int width, unsigned int height, unsigned int curves,
    unsigned int alterations, unsigned int radius, unsigned int twist,
    double line_width)
{
    Spiral *self;
    ParaContext *para;

    self = malloc(sizeof(*self));
    memset(self, 0, sizeof(*self));

    /* width or height may be too large */
    self->data = malloc(width * height);
    if (!self->data) {
        free(self);
        return NULL;
    }

    /* Just copy the attributes */
    self->width = width;
    self->height = height;
    self->curves = curves;
    self->alterations = alterations;
    self->radius = radius;
    self->twist = twist;
    self->line_width = line_width;

    /* Create the texture */
    para = para_create(self, (ParaCallback)spiral_initialize_do);
    para_execute(para, 0, self->height);
    para_free(para);

    return self;
}

void
spiral_free(Spiral *self)
{
    if (!self) {
        return;
    }

    free(self->data);
    free(self);
}

unsigned int
spiral_get_width(Spiral *self)
{
    if (!self) {
        return 0;
    }

    return self->width;
}

unsigned int
spiral_get_height(Spiral *self)
{
    if (!self) {
        return 0;
    }

    return self->height;
}

void*
spiral_get_data(Spiral *self)
{
    if (!self) {
        return 0;
    }

    return self->data;
}
