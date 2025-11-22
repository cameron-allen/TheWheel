#include "pch.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "core/event_handler.h"

EventHandler* EventHandler::mp_instance = nullptr;

EventHandler& EventHandler::GetInstance() 
{
	if (!mp_instance)
		mp_instance = new EventHandler();
	return *mp_instance;
}

void EventHandler::Init() 
{
	SDL_RegisterEvents(GetInstance().m_eventCount);
}

void EventHandler::RegisterEvent(const char* eventName) 
{
	EventHandler& e = GetInstance();
	auto itr = e.m_nameMap.find(eventName);

	if (itr == e.m_nameMap.end())
		e.m_nameMap[eventName] = e.m_eventCount++;
}

void EventHandler::SubToEvent(unsigned int typeId, void(*func)(SDL_Event*))
{
	GetInstance().m_subs[typeId].push_back(func);
}

void EventHandler::SubToEvent(const char* eventName, void(*func)(SDL_Event*))
{
	SubToEvent(GetEventType(eventName), func);
}

void EventHandler::PushEvent(SDL_Event* event)
{
	SDL_PushEvent(event);
}

unsigned int EventHandler::GetEventType(const char* eventName)
{
	EventHandler& e = GetInstance();
	unsigned int eventIndex = 0;

	auto itr = e.m_nameMap.find(eventName);

	if (itr != e.m_nameMap.end())
		eventIndex = itr->second;
	else
		std::cerr << "<EventHandler type ERROR>\n eventName not registered!\n";

	return e.m_userEventsIndex + eventIndex;
}

void EventHandler::InvokeEventSubs(SDL_Event* event)
{
	EventHandler& e = GetInstance();
	auto itr = e.m_subs.find(event->type);

	if (itr != e.m_subs.end())
		for (auto sub : itr->second)
			sub(event);
}
