#ifndef SPIRAL_H
#define SPIRAL_H

typedef struct Spiral Spiral;

/**
 * Initialises the data of a Spiral.
 *
 * @param width, height
 *     The dimensions of the buffer.
 * @param curves
 *     The number of curves that extend from the centre.
 * @param alterations
 *     The number of alterations of direction of the curves.
 * @param radius
 *     The radius of the spiral; pixels further from the centre will be black.
 * @param twist
 *     The twist to apply.
 * @param line_width
 *     The width of the curves; 0.5 means that half of the spiral will be
 *     painted with the foreground colour and half with the background colour.
 */
Spiral*
spiral_create(unsigned int width, unsigned int height, unsigned int curves,
    unsigned int alterations, unsigned int radius, unsigned int twist,
    double line_width);

/**
 * Frees a previously created spiral and all its data.
 *
 * @param self
 *     The spiral to free. If this is NULL, no action is taken.
 */
void
spiral_free(Spiral *self);

/**
 * Returns the width of the spiral.
 *
 * @param self
 *     The spiral whose width to retrieve.
 * @return the width of the spiral, or 0 if spiral is NULL
 */
unsigned int
spiral_get_width(Spiral *self);

/**
 * Returns the height of the spiral.
 *
 * @param self
 *     The spiral whose height to retrieve.
 * @return the height of the spiral, or 0 if spiral is NULL
 */
unsigned int
spiral_get_height(Spiral *self);

/**
 * Returns the data pointer of the spiral texture.
 *
 * The format of the texture is GL_ALPHA8, and its alignment is 1; thus the size
 * of the buffer is spiral_get_height(self) * spiral_get_width(self).
 *
 * @param self
 *     The spiral whose data to retrieve.
 * @return the texture data, or NULL if self is NULL
 */
void*
spiral_get_data(Spiral *self);

#endif
