#pragma once
#include <SDL3/SDL.h>

class SDLWindow 
{
private:
	SDL_Window* mp_window;
	SDL_Event* mp_event;
	bool m_open;
public:
	//@brief Init SDL window
	void init();
	//@brief Clean dynamic memory
	void clean();
	//@brief Returns if window is open
	bool isOpen() const { return m_open; }
	//@brief Flags window to close
	void close() { m_open = false; }
	//@brief Checks and responds to SDL events
	void checkEvents();
};