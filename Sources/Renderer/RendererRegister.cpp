#include "RendererRegister.hpp"

namespace acid
{
	RendererRegister::RendererRegister() :
		m_stages(std::map<GraphicsStage, std::vector<std::unique_ptr<IRenderer>>>())
	{
	}

	IRenderer *RendererRegister::Add(IRenderer *renderer)
	{
		if (renderer == nullptr)
		{
			return nullptr;
		}

		bool emplaced = false;

		do 
		{
			auto stage = m_stages.find(renderer->GetGraphicsStage());

			if (stage == m_stages.end())
			{
				m_stages.emplace(renderer->GetGraphicsStage(), std::vector<std::unique_ptr<IRenderer>>());
			}
			else
			{
				(*stage).second.emplace_back(renderer);
				emplaced = true;
			}
		} while (!emplaced);

		return renderer;
	}

	void RendererRegister::Remove(IRenderer *renderer)
	{
		for (auto &[key, renderers] : m_stages)
		{
			renderers.erase(std::remove_if(renderers.begin(), renderers.end(), [renderer](const std::unique_ptr<IRenderer> &r){
				return renderer == r.get();
			}), renderers.end());
		}
	}
}
