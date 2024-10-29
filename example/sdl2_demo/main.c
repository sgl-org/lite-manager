#include<SDL.h>
#include<stdio.h>


#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240



int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "lm sdl demo", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
    );

    if (!window)
    {
        printf("SDL_CreateWindow error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Surface* screenSurface = SDL_GetWindowSurface(window);
    if (!screenSurface)
    {
        printf("SDL_GetWindowSurface error: %s", SDL_GetError());
        return -1;
    }

    SDL_FillRect(
        screenSurface,
        NULL,
        SDL_MapRGB(
            screenSurface->format,
            0xFF, 0x00, 0x00
        )
    );

    SDL_UpdateWindowSurface(window);
    SDL_Delay(1000);
    printf("demo exit...");
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
