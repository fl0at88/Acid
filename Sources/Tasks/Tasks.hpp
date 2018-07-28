#pragma once

#include <functional>
#include <vector>
#include "Engine/Engine.hpp"

namespace acid
{
	/// <summary>
	/// A module used for managing tasks on engine updates.
	/// </summary>
	class ACID_EXPORT Tasks :
		public IModule
	{
	private:
		std::vector<std::function<void()>> m_tasks;
	public:
		/// <summary>
		/// Gets this engine instance.
		/// </summary>
		/// <returns> The current module instance. </returns>
		static Tasks *Get()
		{
			return Engine::Get()->GetModule<Tasks>();
		}

		Tasks();

		~Tasks();

		void Update() override;

		std::string GetName() const override { return "Tasks"; };

		/// <summary>
		/// Adds an task to the que.
		/// </summary>
		/// <param name="task"> The task to add. </param>
		void AddTask(std::function<void()> task);
	};
}
