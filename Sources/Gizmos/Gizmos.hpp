#pragma once

#include <map>
#include <vector>
#include <memory>
#include "Engine/Engine.hpp"
#include "Gizmo.hpp"

namespace acid
{
	/// <summary>
	/// A module used for that manages debug gizmos.
	/// </summary>
	class ACID_EXPORT Gizmos :
		public Module<Gizmos>
	{
	private:
		std::map<std::shared_ptr<GizmoType>, std::vector<std::unique_ptr<Gizmo>>> m_gizmos;
	public:
		Gizmos();

		Gizmos(const Gizmos&) = delete;

		Gizmos& operator=(const Gizmos&) = delete;

		void Update() override;

		Gizmo *AddGizmo(Gizmo *gizmo);

		bool RemoveGizmo(Gizmo *gizmo);

		/// <summary>
		/// Clears all gizmods from the scene.
		/// </summary>
		void Clear();

		/// <summary>
		/// Gets a list of all gizmods.
		/// </summary>
		/// <returns> All gizmods. </returns>
		const std::map<std::shared_ptr<GizmoType>, std::vector<std::unique_ptr<Gizmo>>> &GetGizmos() const { return m_gizmos; }
	};
}
