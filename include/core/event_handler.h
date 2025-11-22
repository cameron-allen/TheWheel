

class EventHandler 
{
	friend class Core;

private:
	std::unordered_map<unsigned int, std::vector<void (*)(SDL_Event*)>> m_subs;
	std::unordered_map<const char*, unsigned int> m_nameMap;
	unsigned int m_eventCount = 0;
	unsigned int m_userEventsIndex = 0;
	
	static EventHandler* mp_instance;

	//@brief Gets static instance
	static EventHandler& GetInstance();
	//@brief Initializes EventHandler
	static void Init();

public:
	//@brief Registers an event (must be done before Init is called).
	//@brief Only needed for user defined events.
	//@param eventName:	name of event to be registered 
	static void RegisterEvent(const char* eventName);
	//@brief Subscribes a function to an event (event name must already be registered)
	//@param typeId:	event type id (compatible with SDL_EventType) 
	//@param func:	function that will be linked to event
	static void SubToEvent(unsigned int typeId, void (*func)(SDL_Event*));
	//@brief Subscribes a function to an event (event name must already be registered)
	//@param eventName:	name of event being subscribed to 
	//@param func:		function that will be linked to event
	static void SubToEvent(const char* eventName, void (*func)(SDL_Event*));
	//@brief Pushes event to SDL_Event queue
	static void PushEvent(SDL_Event* event);
	//@brief Gets event type id for user defined event
	//@param eventName:	name of event 
	static unsigned int GetEventType(const char* eventName);
	//@brief Invokes all subscriptions to an event
	static void InvokeEventSubs(SDL_Event* event);
	//@brief Deallocates memory
	static void Clean() { delete(mp_instance); }
};