#pragma once

class SDLWindow 
{
private:
	SDL_Window* mp_window;
	SDL_Event* mp_event;
	static bool m_open;

	static void PressKeyCallback(SDL_Event* event);
	static void ResizeWindowCallback(SDL_Event* event);
	static void QuitCallback(SDL_Event* event);

public:
	//@brief Init SDL window
	void init();
	//@brief Clean dynamic memory
	void clean();
	//@brief Flags window to close
	void close() { m_open = false; }
	//@brief Checks and responds to SDL events
	void checkEvents();
	//@brief Returns if window is open
	bool isOpen() { return m_open; }
	//@brief Returns SDL window
	SDL_Window* getWindow() { return mp_window; }
};