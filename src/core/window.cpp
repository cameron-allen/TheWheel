#include "pch.h"
#include "core/window.h"
#include "core/engine.h"
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

    m_open = true;
    mp_event = new SDL_Event();

    // SDL_RegisterEvents(Uint32 events), SDL_PushEvent(SDL_Event* event);
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
        switch(mp_event->type)
        {
        case SDL_EVENT_QUIT:
            close();
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            Core::m_framebufferResized = true;
            std::cout << mp_event->window.data1 << " " << mp_event->window.data2 << std::endl;
            break;
        case SDL_EVENT_KEY_UP:
            switch(mp_event->key.key) 
            {
            case SDLK_ESCAPE:
                if (mp_event->key.key == SDLK_ESCAPE)
                    close();
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
}
