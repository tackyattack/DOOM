#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include <SDL2/SDL.h>
#include <pthread.h>
#include <sys/queue.h>

uint8_t translate_key(SDL_Keycode keycode);

typedef struct _event_item
{
    TAILQ_ENTRY(_event_item)
    nodes;
    SDL_KeyboardEvent keyboard_event;
} event_item;
TAILQ_HEAD(event_queue_s, _event_item)
event_queue;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
pthread_t doom_thread_id;

SDL_Window *window = NULL;
SDL_Texture *sdlTexture = NULL;
SDL_Renderer *gRenderer = NULL;
uint8_t framebuffer[640 * 480 * 4];

static uint8_t *cur_buf;
static uint8_t *audio_pos;
static uint32_t audio_len = 0;
static uint32_t audio_in_pos = 0;
uint8_t audio_buf_A[2048] = {0};
uint8_t audio_buf_B[2048] = {0};

// eventually move these into a their own layer that is a platform
// abstraction.
// One for macOS, etc
void platform_render_to_screen(uint8_t *buf, uint32_t frame_bytes)
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

// queue of stuff that happened
uint8_t platform_get_next_event(event_t *event_out)
{
    event_item *event;
    uint8_t rc = 0;
    event = TAILQ_FIRST(&event_queue);
    if (event != NULL)
    {
        event_out->type = event->keyboard_event.state == SDL_PRESSED ? ev_keydown : ev_keyup;
        event_out->data1 = translate_key(event->keyboard_event.keysym.sym);
        rc = 1;
        TAILQ_REMOVE(&event_queue, event, nodes);
        free(event);
    }
    else
    {
        rc = 0;
    }
    return rc;
}

void platform_write_audio(uint8_t *buf, uint32_t length) {
    uint32_t bytes_left = sizeof(audio_buf_A) - audio_in_pos;
    uint32_t length_to_write = (length > bytes_left ? bytes_left : length);
    memcpy(&cur_buf[audio_in_pos], buf, length_to_write);
    audio_in_pos += length_to_write;
}

uint8_t translate_key(SDL_Keycode keycode)
{
    uint8_t rc;
    switch (keycode)
    {
    case SDLK_LEFT:
        rc = KEY_LEFTARROW;
        break;
    case SDLK_RIGHT:
        rc = KEY_RIGHTARROW;
        break;
    case SDLK_DOWN:
        rc = KEY_DOWNARROW;
        break;
    case SDLK_UP:
        rc = KEY_UPARROW;
        break;
    case SDLK_ESCAPE:
        rc = KEY_ESCAPE;
        break;
    case SDLK_RETURN:
        rc = KEY_ENTER;
        break;
    case SDLK_TAB:
        rc = KEY_TAB;
        break;
    case SDLK_F1:
        rc = KEY_F1;
        break;
    case SDLK_F2:
        rc = KEY_F2;
        break;
    case SDLK_F3:
        rc = KEY_F3;
        break;
    case SDLK_F4:
        rc = KEY_F4;
        break;
    case SDLK_F5:
        rc = KEY_F5;
        break;
    case SDLK_F6:
        rc = KEY_F6;
        break;
    case SDLK_F7:
        rc = KEY_F7;
        break;
    case SDLK_F8:
        rc = KEY_F8;
        break;
    case SDLK_F9:
        rc = KEY_F9;
        break;
    case SDLK_F10:
        rc = KEY_F10;
        break;
    case SDLK_F11:
        rc = KEY_F11;
        break;
    case SDLK_F12:
        rc = KEY_F12;
        break;

    case SDLK_BACKSPACE:
    case SDLK_DELETE:
        rc = KEY_BACKSPACE;
        break;

    case SDLK_PAUSE:
        rc = KEY_PAUSE;
        break;

    case SDLK_KP_EQUALS:
    case SDLK_EQUALS:
        rc = KEY_EQUALS;
        break;

    case SDLK_KP_MEMSUBTRACT:
    case SDLK_MINUS:
        rc = KEY_MINUS;
        break;

    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        rc = KEY_RSHIFT;
        break;
    }
    return rc;
}

void handle_key(SDL_KeyboardEvent key)
{
    event_item *event = (event_item *)malloc(sizeof(event_item));
    event->keyboard_event = key;
    TAILQ_INSERT_TAIL(&event_queue, event, nodes);
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
                if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
                {
                    handle_key(e.key);
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

void audio_callback(void *unused, uint8_t *stream, int len)
{
    if (audio_len == 0)
    {
        audio_len = audio_in_pos;
        if(cur_buf == audio_buf_A) {
            cur_buf = audio_buf_B;
        } else {
            cur_buf = audio_buf_A;
        }
        audio_in_pos = 0;
        audio_pos = cur_buf;
    }

    len = (len > audio_len ? audio_len : len);
    SDL_memcpy (stream, audio_pos, len); 
    //SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

static void init_sound(void)
{
    cur_buf = audio_buf_A;
    SDL_AudioSpec fmt;
    fmt.freq = 11025;
    fmt.format = AUDIO_S16;
    fmt.channels = 2;
    fmt.samples = 512;
    fmt.callback = audio_callback;
    fmt.userdata = NULL;

    /* Open the audio device and start playing sound! */
    if (SDL_OpenAudio(&fmt, NULL) < 0)
    {
        fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_PauseAudio(0);
}

int main(int argc,
         char **argv)
{
    myargc = argc;
    myargv = argv;
    TAILQ_INIT(&event_queue);
    init_sound();
    start_app();
    sdl_loop();
    SDL_CloseAudio();
    return 0;
}
