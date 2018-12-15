#include "Engine.hpp"

namespace acid
{
	using HighResolutionClock = std::chrono::high_resolution_clock;
	using MicrosecondsType = std::chrono::duration<int64_t, std::micro>;

	Engine *Engine::INSTANCE = nullptr;
	std::chrono::time_point<HighResolutionClock> TIME_START = HighResolutionClock::now();

	Engine::Engine(const bool &emptyRegister) :
		m_moduleManager(ModuleManager(emptyRegister)),
		m_running(true),
		m_error(false),
		m_timeOffset(Time::ZERO),
		m_fpsLimit(-1.0f)
	{
		INSTANCE = this;
	}

	Engine::~Engine()
	{
		Log::CreateLog("Logs/" + ((m_error ? "Crashes/" : "") + GetDateTime()) + ".log");
	}

	int32_t Engine::Run()
	{
		while (m_running)
		{
			// TODO: Catch exceptions.
			m_moduleManager.Update();
		}

		return EXIT_SUCCESS;
	}

	void Engine::RequestClose(const bool &error)
	{
		m_running = false;

		// A statement in case it was already true.
		if (error)
		{
			m_error = true;
		}
	}

	Time Engine::GetTime()
	{
		auto duration = Time::Microseconds(std::chrono::duration_cast<MicrosecondsType>(HighResolutionClock::now() - TIME_START).count());

		if (INSTANCE == nullptr)
		{
			return duration;
		}

		return duration + INSTANCE->m_timeOffset;
	}

	std::string Engine::GetDateTime()
	{
		time_t rawtime;
		struct tm *timeinfo;
		char buffer[80];

		time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer, sizeof(buffer), "%Y-%m-%d-%I%M%S", timeinfo);
		std::string str = std::string(buffer);
		return str;
	}
}
