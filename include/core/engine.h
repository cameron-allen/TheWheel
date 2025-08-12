#pragma once

class SDLWindow;

class Core 
{
private:
	SDLWindow* mp_window;
public:
	//@brief Run engine
	void run();
	//@brief Init data members
	void init();
	//@brief Core engine loop
	void loop();
	//@brief Clean engine data members
	void clean();
};