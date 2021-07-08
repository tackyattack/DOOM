#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include <SDL2/SDL.h>
#include <pthread.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
pthread_t doom_thread_id;

SDL_Window *window = NULL;
SDL_Texture *sdlTexture = NULL;
SDL_Renderer *gRenderer = NULL;
uint8_t framebuffer[640 * 480 * 4];

void render_to_screen(uint8_t *buf, uint32_t frame_bytes)
{
    if (sdlTexture == NULL || window == NULL || gRenderer == NULL)
    {
        return;
    }
    memcpy(framebuffer, buf, frame_bytes);
    SDL_UpdateTexture(sdlTexture, NULL, framebuffer, 640 * 4);
    SDL_RenderClear(gRenderer);
    SDL_RenderCopy(gRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(gRenderer);
}

static void sdl_loop(void)
{

    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        //Create window
        window = SDL_CreateWindow("DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }

        // https://wiki.libsdl.org/MigrationGuide
        gRenderer = SDL_CreateRenderer(window, -1, 0);
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

        sdlTexture = SDL_CreateTexture(gRenderer,
                                       SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       640, 480);

        SDL_Event e;
        uint8_t quit = false;
        while (!quit)
        {
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                {
                    quit = true;
                }
                if (e.type == SDL_KEYDOWN)
                {
                    quit = true;
                }
                if (e.type == SDL_MOUSEBUTTONDOWN)
                {
                    quit = true;
                }
            }
        }
    }
}

static void *app_thread(void *vargp)
{
    D_DoomMain();
}

static void start_app(void)
{
    pthread_create(&doom_thread_id, NULL, app_thread, NULL);
}

int main(int argc,
         char **argv)
{
    myargc = argc;
    myargv = argv;
    start_app();
    sdl_loop();
    return 0;
}
