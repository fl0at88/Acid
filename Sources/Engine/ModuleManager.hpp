#pragma once

#include <map>
#include <memory>
#include "Module.hpp"
#include "Maths/Delta.hpp"
#include "Maths/Timer.hpp"

namespace acid
{
	/// <summary>
	/// A class that contains, manages, and updates engine modules.
	/// </summary>
	class ACID_EXPORT ModuleManager
	{
	private:
		std::map<float, std::unique_ptr<IModule>> m_modules;
		Delta m_deltaUpdate;
		Delta m_deltaRender;
		Timer m_timerUpdate;
		Timer m_timerRender;
	public:
		/// <summary>
		/// Creates a module manager for a engine.
		/// </summary>
		/// <param name="emptyRegister"> If the module register will start empty. </param>
		ModuleManager(const bool &emptyRegister = false);

		ModuleManager(const ModuleManager&) = delete;

		ModuleManager& operator=(const ModuleManager&) = delete;

		/// <summary>
		/// Updates all modules in this manager in order.
		/// </summary>
		void Update();

		/// <summary>
		/// Gets if a module is contained in this registry.
		/// </summary>
		/// <param name="module"> The module to find. </param>
		/// <returns> If the module is in the registry. </returns>
		bool Contains(IModule *module) const;

		/// <summary>
		/// Gets a module instance by type from the register.
		/// </summary>
		/// <param name="T"> The module type to find. </param>
		/// <returns> The found module. </returns>
		template<class T>
		T *Get() const
		{
			for (auto &[key, module] : m_modules)
			{
				auto casted = dynamic_cast<T *>(module.get());

				if (casted != nullptr)
				{
					return casted;
				}
			}

			return nullptr;
		}

		/// <summary>
		/// Adds a module to the manager.
		/// </summary>
		/// <param name="module"> The modules object. </param>
		/// <param name="update"> The modules update type. </param>
		/// <returns> The module that has been added. </returns>
		IModule *Add(IModule *module, const ModuleUpdate &update);

		/// <summary>
		/// Adds a module to the manager.
		/// </summary>
		/// <param name="update"> The modules update type. </param>
		/// <param name="T"> The modules type. </param>
		/// <returns> The created module. </returns>
		template<class T>
		T *Add(const ModuleUpdate &update)
		{
			auto module = static_cast<T *>(malloc(sizeof(T)));
			Module<T>::INSTANCE = module;
			Module<T>::UPDATE = update;
			Add(static_cast<IModule *>(module), update);
			new(module) T();
			return module;
		}

		/// <summary>
		/// Removes a module from the manager and deletes the instance.
		/// </summary>
		/// <param name="module"> The module to remove. </param>
		void Remove(IModule *module);

		/// <summary>
		/// Removes a module from the manager and deletes the instance.
		/// </summary>
		/// <param name="T"> The type of module to remove. </param>
		template<class T>
		void Remove()
		{
		//	m_modules.erase(std::remove_if(m_modules.begin(), m_modules.end(), [module](const float &i, const std::unique_ptr<IModule> &m){
		//		auto casted = dynamic_cast<T *>(m.get());
		//		return casted != nullptr; // FIXME: Remove
		//	}), m_modules.end());
		}

		/// <summary>
		/// Gets the delta between updates.
		/// </summary>
		/// <returns> The delta between updates. </returns>
		Time GetDelta() const { return m_deltaUpdate.GetChange(); }

		/// <summary>
		/// Gets the delta between renders.
		/// </summary>
		/// <returns> The delta between renders. </returns>
		Time GetDeltaRender() const { return m_deltaRender.GetChange(); }
	private:
		/// <summary>
		/// Runs updates for all module update types.
		/// </summary>
		/// <param name="update"> The modules update type. </param>
		void RunUpdate(const ModuleUpdate &update) const;
	};
}
