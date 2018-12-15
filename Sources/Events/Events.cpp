#include "Events.hpp"

#include <algorithm>

namespace acid
{
	Events::Events() :
		m_events(std::vector<std::unique_ptr<IEvent>>())
	{
	}

	void Events::Update()
	{
		for (auto it = m_events.begin(); it != m_events.end();)
		{
			if (!(*it)->EventTriggered())
			{
				++it;
				continue;
			}

			(*it)->OnEvent();

			if ((*it)->RemoveAfterEvent())
			{
				it = m_events.erase(it);
				continue;
			}

			++it;
		}
	}

	IEvent *Events::AddEvent(IEvent *event)
	{
		m_events.emplace_back(event);
		return event;
	}

	void Events::RemoveEvent(IEvent *event)
	{
		m_events.erase(std::remove_if(m_events.begin(), m_events.end(), [event](const std::unique_ptr<IEvent> &e){
			return event == e.get();
		}), m_events.end());
	}
}
