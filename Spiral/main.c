#include <math.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL.h>

#define ARGUMENTS_NO_SETUP
#define ARGUMENTS_NO_TEARDOWN
#include "arguments/arguments.h"

#include "spiral.h"

/**
 * The user event code that signals that the display should be refreshed.
 */
#define USER_EVENT_DISPLAY (SDL_USEREVENT + 1)

/**
 * The number of milliseconds between each redraw.
 */
#define TIMER_INTERVAL 40

/**
 * The number of alterations for the spiral.
 */
#define SPIRAL_ALTERATIONS ARGUMENT_VALUE(spiral_alterations)

/**
 * The number of curves for the spiral.
 */
#define SPIRAL_CURVES ARGUMENT_VALUE(spiral_curves)

/**
 * The line width for the spiral.
 */
#define SPIRAL_LINE_WIDTH ARGUMENT_VALUE(spiral_line_width)

/**
 * The rotation speed of the spiral in cycles per second.
 */
#define SPIRAL_ROTATION_SPEED ARGUMENT_VALUE(spiral_rotation_speed)

/**
 * The amount of twist to apply to the curves.
 */
#define SPIRAL_TWIST ARGUMENT_VALUE(spiral_twist)

/**
 * The number of horizontal nodes in the animated background.
 */
#define ANIMATION_WIDTH ARGUMENT_VALUE(background_animation_size).width

/**
 * The number of vertical nodes in the animated background.
 */
#define ANIMATION_HEIGHT ARGUMENT_VALUE(background_animation_size).height

/**
 * The speed of the background animation.
 */
#define ANIMATION_SPEED ARGUMENT_VALUE(background_animation_speed)

/**
 * The turbulence of the background animation.
 */
#define ANIMATION_TURBULENCE ARGUMENT_VALUE(background_animation_turbulence)

/**
 * The opacity of the background animation.
 */
#define ANIMATION_OPACITY 0.4

/**
 * One node of the animated background.
 */
typedef struct {
    /** The colour of the node */
    GLfloat red, green, blue;

    /** The random offset of the node */
    double d;
} AnimationNode;

static struct {
    /** The scale factor to apply to make horisontal and vertical distances
        equal */
    GLfloat xscale, yscale;

    struct {
        /** The size of the texture expressed as the width in pixels; the
            texture is square */
        unsigned int size;

        /** The identifier for the spiral texture */
        GLuint texture;

        /** The scale factor used to zoom into the actual spiral */
        GLfloat scale;
    } spiral;

    struct {
        /** The width and height, in nodes, of the animation */
        unsigned int width, height;

        /** The animation nodes; this array will contain width * height
            elements */
        AnimationNode *nodes;
    } animation;
} context;

/**
 * Returns the opacity of a colour value based on the current time.
 *
 * @param t
 *     The current time, expressed as seconds since the first frame.
 * @return the opacity of the colour expressed as a value between 0.0 and 1.0
 */
static inline double
color_function(double t)
{
    return (sin(ANIMATION_SPEED * t * 2.0 * M_PI) + 1.0) / 2.0;
}

/**
 * Returns the amount of skew to apply to a coordinate of the background
 * animation based on the current time.
 *
 * @param t
 *     The current time, expressed as seconds since the first frame. The random
 *     offset of the node should be added to this value.
 * @return the skew
 */
static inline double
skew_function(double t)
{
    return ANIMATION_TURBULENCE * sin(ANIMATION_SPEED * t);
}

/**
 * Returns a node of the animated background.
 *
 * @param x, y
 *     The coordinates of the node to get. The coordinates are clipped to the
 *     dimensions of the animated background before being used.
 * @return the node at (x, y)
 */
static inline AnimationNode*
context_animation_get_node(int x, int y)
{
    if (x < 0) {
        x = 0;
    }
    else if (x >= context.animation.width - 1) {
        x = context.animation.width - 1;
    }
    if (y < 0) {
        y = 0;
    }
    else if (y >= context.animation.height - 1) {
        y = context.animation.height - 1;
    }

    return &context.animation.nodes[y * context.animation.width + x];
}

/**
 * Initialises the animation struct of context.
 *
 * If this function returns successfully, context_animation_free must be called.
 *
 * @return non-zero if the animation struct was sucessfully initialised and 0
 *     otherwise
 * @see context_animation_free
 */
static int
context_animation_init(void)
{
    context.animation.width = ANIMATION_WIDTH;
    context.animation.height = ANIMATION_HEIGHT;

    /* Allocate a buffer for the nodes and fail if that fails */
    context.animation.nodes = malloc(sizeof(*context.animation.nodes)
        * context.animation.width * context.animation.height);
    if (!context.animation.nodes) {
        printf("Failed to create animation of size %dx%d.\n",
            context.animation.width, context.animation.height);
        return 0;
    }

    /* Initialise the nodes */
    int x, y;
    for (x = 0; x < context.animation.width; x++) {
        for (y = 0; y < context.animation.height; y++) {
            AnimationNode *node = context_animation_get_node(x, y);
            node->red = (GLfloat)rand() / RAND_MAX;
            node->green = (GLfloat)rand() / RAND_MAX;
            node->blue = (GLfloat)rand() / RAND_MAX;
            node->d = 2.0 * M_PI * (double)rand() / RAND_MAX;
        }
    }

    return 1;
}

/**
 * Releases the resouces allocated by context_animation_init.
 */
static void
context_animation_free(void)
{
    free(context.animation.nodes);
}

/**
 * Draws the animated background.
 *
 * @param t
 *     The current time, expressed as seconds since the first frame.
 */
static void
context_animation_render(double t)
{
    /* Do nothing if the animation will not be visible */
    if (ANIMATION_OPACITY <= 0.0) {
        return;
    }

    glPushMatrix();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Make sure that the loops below iterate over the entire screen; we keep a
       margin so that all drawn squares are animated */
    glScalef(
        context.xscale * 2.0 / (context.animation.width - 1.5),
        context.yscale * 2.0 / (context.animation.height - 1.5),
        1.0);

    int x, y;
    for (y = 0; y < context.animation.height; y++) {
        for (x = 0; x < context.animation.width; x++) {
            AnimationNode *a;

            glPushMatrix();

            /* Move to the correct square */
            glTranslatef(
                x - 0.5 * context.animation.width,
                y - 0.5 * context.animation.height,
                0.0);

            glBegin(GL_QUADS);

            /* Top left */
            a = context_animation_get_node(x, y);
            glColor4f(a->red, a->green, a->blue,
                ANIMATION_OPACITY * color_function(t + a->d));
            glVertex2f(
                (x
                    ? skew_function(t + a->d + M_PI / 2.0)
                    : 0.0),
                (y
                    ? skew_function(t + a->d)
                    : 0.0));

            /* Bottom left */
            a = context_animation_get_node(x, y + 1);
            glColor4f(a->red, a->green, a->blue,
                ANIMATION_OPACITY * color_function(t + a->d));
            glVertex2f(
                (x
                    ? skew_function(t + a->d + M_PI / 2.0)
                    : 0.0),
                1.0 + (y < context.animation.height - 1
                    ? skew_function(t + a->d)
                    : 0.0));

            /* Bottom right */
            a = context_animation_get_node(x + 1, y + 1);
            glColor4f(a->red, a->green, a->blue,
                ANIMATION_OPACITY * color_function(t + a->d));
            glVertex2f(
                1.0 + (x < context.animation.width - 1
                    ? skew_function(t + a->d + M_PI / 2.0)
                    : 0.0),
                1.0 + (y < context.animation.height - 1
                    ? skew_function(t + a->d)
                    : 0.0));

            /* Top right */
            a = context_animation_get_node(x + 1, y);
            glColor4f(a->red, a->green, a->blue,
                ANIMATION_OPACITY * color_function(t + a->d));
            glVertex2f(
                1.0 + (x < context.animation.width - 1
                    ? skew_function(t + a->d + M_PI / 2.0)
                    : 0.0),
                (y
                    ? skew_function(t + a->d)
                    : 0.0));

            glEnd();

            glPopMatrix();
        }
    }

    glDisable(GL_BLEND);

    glPopMatrix();
}

/**
 * Creates the spiral texture and initialises the spiral struct of context.
 *
 * If this function returns successfully, context_spiral_free must be called.
 *
 * @param viewport_width, viewport_height
 *     The dimensions of the spiral viewport.
 * @return non-zero if the spiral struct was sucessfully initialised and 0
 *     otherwise
 * @see context_spiral_free
 */
static int
context_spiral_init(unsigned int viewport_width, unsigned int viewport_height)
{
    /* Calculate the spiral texture size; it must be a power of two, and we
       allow a maximum of 2^16 */
    int i;
    unsigned int radius, spiral_size;
    radius = (unsigned int)hypot(viewport_width * 0.5, viewport_height * 0.5);
    spiral_size = 2 * (radius + 1);
    for (i = 16; i; i--) {
        if ((1 << i) & spiral_size) {
            if (((1 << i) - 1) & spiral_size) {
                context.spiral.size =  1 << (i + 1);
            }
            else {
                context.spiral.size = spiral_size;
            }
            break;
        }
    }

    /* Create the spiral */
    Spiral *spiral = spiral_create(context.spiral.size, context.spiral.size,
        SPIRAL_CURVES, SPIRAL_ALTERATIONS, radius, SPIRAL_TWIST,
        SPIRAL_LINE_WIDTH);
    if (!spiral) {
        printf("Failed to create spiral of size %dx%d.\n", context.spiral.size,
            context.spiral.size);
        return 0;
    }

    /* Calculate the scale factors */
    context.spiral.scale = (GLfloat)context.spiral.size / spiral_size
        * (2.0 * radius
            / (viewport_width < viewport_height
                ? viewport_width : viewport_height));

    /* Bind the data to a texture */
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &context.spiral.texture);
    glBindTexture(GL_TEXTURE_2D, context.spiral.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, spiral_get_width(spiral),
        spiral_get_height(spiral), 0, GL_ALPHA, GL_UNSIGNED_BYTE,
        spiral_get_data(spiral));
    glDisable(GL_TEXTURE_2D);

    /* Free the spiral, since we do not need it any more */
    spiral_free(spiral);

    return 1;
}

/**
 * Releases the resouces allocated by context_spiral_init.
 */
static void
context_spiral_free(void)
{
    glDeleteTextures(1, &context.spiral.texture);
}

/**
 * Renders the spiral.
 *
 * @param t
 *     The current time, expressed as seconds since the first frame.
 */
static void
context_spiral_render(double t)
{
    glPushMatrix();

    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    /* Set the rotation relative to the current time */
    glRotated(-360 * SPIRAL_ROTATION_SPEED * t, 0.0, 0.0, 1.0);

    /* Make sure that the transparent padding is not visible */
    glScalef(context.spiral.scale, context.spiral.scale, 1.0);

    /* Draw a rectangle with the spiral as texture */
    glColor3f(
        ARGUMENT_VALUE(spiral_color).d[0],
        ARGUMENT_VALUE(spiral_color).d[1],
        ARGUMENT_VALUE(spiral_color).d[2]);
    glBegin(GL_QUADS);

    glTexCoord2f(0.0, 0.0);
    glVertex2f(-1.0, -1.0);

    glTexCoord2f(0.0, 1.0);
    glVertex2f(-1.0, 1.0);

    glTexCoord2f(1.0, 1.0);
    glVertex2f(1.0, 1.0);

    glTexCoord2f(1.0, 0.0);
    glVertex2f(1.0, -1.0);

    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

/**
 * The timer callback function.
 *
 * This function pushes an event to SDL to make it execute in the main thread.
 *
 * @param interval
 *     The timer interval.
 * @param dummy
 *     Not used.
 * @return the next timer interval, which is the same as the current
 */
static Uint32
do_timer(Uint32 interval, void *dummy)
{
    SDL_Event event;

    event.user.type = SDL_USEREVENT;
    event.user.code = USER_EVENT_DISPLAY;
    event.user.data1 = NULL;
    event.user.data2 = NULL;

    SDL_PushEvent(&event);

    return interval;
}

/**
 * Updates the display.
 */
static void
do_display(void)
{
    static Uint32 start_ticks = 0;
    Uint32 current_ticks = SDL_GetTicks();

    if (!start_ticks) {
        start_ticks = current_ticks;
    }

    double t = (double)(current_ticks - start_ticks) / 1000.0;

    /* Make sure the background is cleared */
    glClearColor(
        ARGUMENT_VALUE(background_color).d[0],
        ARGUMENT_VALUE(background_color).d[1],
        ARGUMENT_VALUE(background_color).d[2],
        0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-context.xscale, context.xscale, -context.yscale, context.yscale,
        0.0, 1.0);

    context_animation_render(t);
    context_spiral_render(t);

    /* Render to screen */
    SDL_GL_SwapBuffers();
}

/**
 * Handles any pending SDL events.
 *
 * @return non-zero if the application should continue running and 0 otherwise
 */
static int
handle_events(void)
{
    SDL_Event event;

    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
        /* Exit if the window is closed */
        case SDL_QUIT:
            return 0;

        /* Check for keypresses */
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                return 0;

            /* Prevent compiler warning */
            default: break;
            }

        case SDL_USEREVENT:
            switch (event.user.code) {
            case USER_EVENT_DISPLAY:
                do_display();
                break;

            default: break;
            }
            break;

        /* Prevent compiler warning */
        default: break;
        }
    }

    return 1;
}

/**
 * Initialises OpenGL for the specified resolution.
 *
 * @param width, height
 *     The width and height of the viewport.
 */
static void
opengl_initialize(int width, int height)
{
    /* Setup our viewport */
    glViewport(0, 0, width, height);
}

static int
main(int argc, char *argv[],
    window_size_t window_size,
    unsigned int spiral_alterations,
    unsigned int spiral_curves,
    double spiral_line_width,
    double spiral_rotation_speed,
    double spiral_twist,
    spiral_color_t spiral_color,
    background_color_t background_color,
    background_animation_size_t background_animation_size,
    double background_animation_speed,
    double background_animation_turbulence)
{
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    /* Hide the mouse cursor */
    SDL_ShowCursor(0);

    /* Get video information */
    const SDL_VideoInfo *vinfo = SDL_GetVideoInfo();
    if (!vinfo) {
        printf("Unable to get video info: %s\n", SDL_GetError());
        return 1;
    }

    /* Initialise the screen */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface* screen;
    unsigned int viewport_width, viewport_height;
    if (window_size.width > 0 && window_size.height > 0) {
        screen = SDL_SetVideoMode(window_size.width, window_size.height,
            32, SDL_OPENGL);
        viewport_width = window_size.width;
        viewport_height = window_size.height;
    }
    else {
        screen = SDL_SetVideoMode(vinfo->current_w, vinfo->current_h,
            32, SDL_OPENGL | SDL_FULLSCREEN);
        viewport_width = vinfo->current_w;
        viewport_height = vinfo->current_h;
    }
    if (!screen) {
        printf("Unable to set %dx%d video: %s\n",
            viewport_width, viewport_height, SDL_GetError());
        return 1;
    }

    /* Intialise GLEW */
    GLenum glew_error = glewInit();
    if (glew_error != GLEW_OK) {
        printf("Failed to intialise GLEW: %s\n",
            glewGetErrorString(glew_error));
        return 1;
    }

    /* Setup OpenGL */
    opengl_initialize(viewport_width, viewport_height);

    /* Create the timer */
    SDL_TimerID timer = SDL_AddTimer(TIMER_INTERVAL, do_timer, NULL);
    if (!timer) {
        printf("Unable to add timer.\n");
        return 1;
    }

    /* Make sure horisontal and vertical distances are equal */
    if (viewport_width > viewport_height) {
        context.xscale = (double)viewport_width / viewport_height;
        context.yscale = 1.0;
    }
    else {
        context.xscale = 1.0;
        context.yscale = (double)viewport_height / viewport_width;
    }

    if (!context_animation_init()) {
        /* context_animation_init prints its own error message */
        return 1;
    }

    if (!context_spiral_init(viewport_width, viewport_height)) {
        /* context_spiral_init prints its own error message */
        return 1;
    }

    /* Enter the main loop */
    while (handle_events());

    /* Release resources */
    context_spiral_free();
    context_animation_free();

    SDL_RemoveTimer(timer);

    return 0;
}
