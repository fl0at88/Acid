#pragma once

#include "Exports.hpp"

namespace acid
{
	/// <summary>
	/// Represents when a module will update in the game loop.
	/// </summary>
	enum ModuleUpdate
	{
		MODULE_UPDATE_NULL = 0,
		MODULE_UPDATE_ALWAYS = 1,
		MODULE_UPDATE_PRE = 2,
		MODULE_UPDATE_NORMAL = 3,
		MODULE_UPDATE_POST = 4,
		MODULE_UPDATE_RENDER = 5
	};

	/// <summary>
	/// A precursor to the <seealso cref="Module"/> object.
	/// </summary>
	class IModule
	{
	public:
		/// <summary>
		/// The update function for the module.
		/// </summary>
		virtual void Update() = 0;
	};

	/// <summary>
	/// A interface used for defining engine modules.
	/// </summary>
	template<class T>
	class Module :
		public IModule
	{
	private:
		friend class ModuleManager;

		static T *INSTANCE;
		static ModuleUpdate UPDATE;
	public:
		/// <summary>
		/// Gets the managed instance of this module.
		/// </summary>
		/// <returns> The current module instance. </returns>
		static T *Get() { return INSTANCE; }

		/// <summary>
		/// Gets when this module will update in the game loop.
		/// </summary>
		/// <returns> The when the module will update. </returns>
		static ModuleUpdate GetUpdate() { return UPDATE; }
	};

	template<class T>
	T *Module<T>::INSTANCE = nullptr;

	template<class T>
	ModuleUpdate Module<T>::UPDATE = MODULE_UPDATE_NULL;
}
