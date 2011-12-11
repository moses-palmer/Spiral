#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL.h>

#define ARGUMENTS_NO_SETUP
#define ARGUMENTS_NO_TEARDOWN
#include "arguments/arguments.h"

/**
 * The user event code that signals that the display should be refreshed.
 */
#define USER_EVENT_DISPLAY (SDL_USEREVENT + 1)

/**
 * The number of milliseconds between each redraw.
 */
#define TIMER_INTERVAL 40

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
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    /* Render to screen */
    SDL_GL_SwapBuffers();

    (void)t;
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
    window_size_t window_size)
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

    /* Enter the main loop */
    while (handle_events());

    SDL_RemoveTimer(timer);

    return 0;
}
