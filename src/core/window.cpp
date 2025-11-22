#include "pch.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "core/window.h"
#include "core/engine.h"
#include "core/event_handler.h"

bool SDLWindow::m_open = false;

void SDLWindow::PressKeyCallback(SDL_Event* event)
{
    switch(event->key.key) 
    {
    case SDLK_ESCAPE:
        m_open = false;
        break;
    default:
        break;
    }
}

void SDLWindow::ResizeWindowCallback(SDL_Event* event)
{
    Core::m_framebufferResized = true;
    std::cout << event->window.data1 << " " << event->window.data2 << std::endl;
}

void SDLWindow::QuitCallback(SDL_Event* event)
{
    m_open = false;
}

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

    EventHandler::SubToEvent(SDL_EVENT_KEY_DOWN, PressKeyCallback);
    EventHandler::SubToEvent(SDL_EVENT_WINDOW_RESIZED, ResizeWindowCallback);
    EventHandler::SubToEvent(SDL_EVENT_QUIT, QuitCallback);
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
        /*switch(mp_event->type)
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
        }*/

        EventHandler::InvokeEventSubs(mp_event);
    }
}
