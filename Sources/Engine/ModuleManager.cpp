#include "ModuleManager.hpp"

#include "Log.hpp"
#include "Audio/Audio.hpp"
#include "Display/Display.hpp"
#include "Events/Events.hpp"
#include "Files/Files.hpp"
#include "Gizmos/Gizmos.hpp"
#include "Inputs/Joysticks.hpp"
#include "Inputs/Keyboard.hpp"
#include "Inputs/Mouse.hpp"
#include "Particles/Particles.hpp"
#include "Renderer/Renderer.hpp"
#include "Resources/Resources.hpp"
#include "Scenes/Scenes.hpp"
#include "Shadows/Shadows.hpp"
#include "Uis/Uis.hpp"

namespace acid
{
	ModuleManager::ModuleManager(const bool &emptyRegister) :
		m_modules(std::map<float, std::unique_ptr<IModule>>()),
		m_deltaUpdate(Delta()),
		m_deltaRender(Delta()),
		m_timerUpdate(Timer(Time::Seconds(1.0f / 68.0f))),
		m_timerRender(Timer(Time::Seconds(1.0f / -1.0f)))
	{
		if (!emptyRegister)
		{
			Add<Display>(MODULE_UPDATE_POST);
			Add<Joysticks>(MODULE_UPDATE_PRE);
			Add<Keyboard>(MODULE_UPDATE_PRE);
			Add<Mouse>(MODULE_UPDATE_PRE);
			Add<Audio>(MODULE_UPDATE_PRE);
			Add<Files>(MODULE_UPDATE_PRE);
			Add<Scenes>(MODULE_UPDATE_NORMAL);
			Add<Gizmos>(MODULE_UPDATE_NORMAL);
			Add<Renderer>(MODULE_UPDATE_RENDER);
			Add<Resources>(MODULE_UPDATE_PRE);
			Add<Events>(MODULE_UPDATE_ALWAYS);
			Add<Uis>(MODULE_UPDATE_PRE);
			Add<Particles>(MODULE_UPDATE_NORMAL);
			Add<Shadows>(MODULE_UPDATE_NORMAL);
		}
	}

	void ModuleManager::Update()
	{
		m_timerRender.SetInterval(Time::Seconds(1.0f / Engine::Get()->GetFpsLimit()));

		// Always-Update.
		RunUpdate(MODULE_UPDATE_ALWAYS);

		if (m_timerUpdate.IsPassedTime())
		{
			// Resets the timer.
			m_timerUpdate.ResetStartTime();

			// Pre-Update.
			RunUpdate(MODULE_UPDATE_PRE);

			// Update.
			RunUpdate(MODULE_UPDATE_NORMAL);

			// Post-Update.
			RunUpdate(MODULE_UPDATE_POST);

			// Updates the engines delta.
			m_deltaUpdate.Update();
		}

		// Prioritize updates over rendering.
		//	if (!Maths::AlmostEqual(m_timerUpdate.GetInterval().AsSeconds(), m_deltaUpdate.GetChange().AsSeconds(), 5.0f))
		//	{
		//		return;
		//	}

		// Renders when needed.
		if (m_timerRender.IsPassedTime() || Engine::Get()->GetFpsLimit() <= 0.0f)
		{
			// Resets the timer.
			m_timerRender.ResetStartTime();

			// Render
			RunUpdate(MODULE_UPDATE_RENDER);

			// Updates the render delta, and render time extension.
			m_deltaRender.Update();
		}
	}

	bool ModuleManager::Contains(IModule *module) const
	{
	//	return std::any_of(m_modules.begin(), m_modules.end(), [module](float f, IModule *m){
	//		return module == m;
	//	});
		for (auto &module1 : m_modules)
		{
			if (module1.second.get() == module)
			{
				return true;
			}
		}
		return false;
	}

	IModule *ModuleManager::Add(IModule *module, const ModuleUpdate &update)
	{
		if (Contains(module))
		{
			Log::Error("Attempting to register a module that has already been registered!\n");
			return nullptr;
		}

		float key = static_cast<float>(update) + (0.01f * static_cast<float>(m_modules.size()));
		m_modules.emplace(key, module);
		return module;
	}

	void ModuleManager::Remove(IModule *module)
	{
	//	m_modules.erase(std::remove_if(m_modules.begin(), m_modules.end(), [module](const float &i, const std::unique_ptr<IModule> &m){
	//		return module == m.get(); // FIXME: Remove
	//	}), m_modules.end());
	}

	void ModuleManager::RunUpdate(const ModuleUpdate &update) const
	{
		for (auto &[key, module] : m_modules)
		{
			if (static_cast<int32_t>(std::floor(key)) == update)
			{
				module->Update();
			}
		}
	}
}
