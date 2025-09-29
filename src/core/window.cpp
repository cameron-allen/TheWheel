#include "pch.h"
#include "core/window.h"
#include <Windows.h>

void SDLWindow::init() 
{
    // Initialize SDL (video subsystem only)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        throw SDL_GetError();
    }

    // Create a window
    mp_window = SDL_CreateWindow(
        "The Wheel",                                // title
        800,                                        // width
        600,                                        // height
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN    // flags (resizable, etc.)
    );

    if (!mp_window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        throw SDL_GetError();
    }

    // Simple loop to keep window open until close event
    m_open = true;
    mp_event = new SDL_Event();
}

void SDLWindow::clean() 
{
    SDL_DestroyWindow(mp_window);
    SDL_Quit();
    delete(mp_event);
}

void SDLWindow::checkEvents()
{
    while (SDL_PollEvent(mp_event))
    {
        if (mp_event->type == SDL_EVENT_QUIT)
            close();
    }
}
