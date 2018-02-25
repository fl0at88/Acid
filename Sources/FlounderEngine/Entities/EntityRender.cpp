#include "EntityRender.hpp"

#include "../Devices/Display.hpp"
#include "../Meshes/Mesh.hpp"
#include "../Materials/Material.hpp"
#include "../Physics/Rigidbody.hpp"
#include "../Scenes/Scenes.hpp"
#include "UbosEntities.hpp"

namespace Flounder
{
	EntityRender::EntityRender() :
		Component(),
		m_uniformObject(new UniformBuffer(sizeof(UbosEntities::UboObject)))
	{
	}

	EntityRender::~EntityRender()
	{
		delete m_uniformObject;
	}

	void EntityRender::Update()
	{
	}

	void EntityRender::CmdRender(const VkCommandBuffer &commandBuffer, const Pipeline &pipeline, const UniformBuffer &uniformScene)
	{
		const auto logicalDevice = Display::Get()->GetLogicalDevice();
		const auto descriptorSet = pipeline.GetDescriptorSet();

		// Gets required components.
		auto mesh = GetGameObject()->GetComponent<Mesh>();
		auto material = GetGameObject()->GetComponent<Material>();
		auto rigidbody = GetGameObject()->GetComponent<Rigidbody>();

		if (mesh == nullptr || mesh->GetModel() == nullptr || material == nullptr)
		{
			return;
		}

		if (rigidbody != nullptr && rigidbody->GetCollider() != nullptr)
		{
			if (!rigidbody->GetCollider()->InFrustum(*Scenes::Get()->GetCamera()->GetViewFrustum()))
			{
				return;
			}
		}

		// Creates a UBO object and write descriptor.
		UbosEntities::UboObject uboObject = {};
		std::vector<VkWriteDescriptorSet> descriptorWrites = std::vector<VkWriteDescriptorSet>();

		descriptorWrites.push_back(uniformScene.GetWriteDescriptor(0, descriptorSet));
		descriptorWrites.push_back(m_uniformObject->GetWriteDescriptor(1, descriptorSet));

		GetGameObject()->GetTransform()->GetWorldMatrix(&uboObject.transform);

		if (material->GetTextureDiffuse() != nullptr)
		{
			descriptorWrites.push_back(material->GetTextureDiffuse()->GetWriteDescriptor(2, descriptorSet));
			uboObject.samples.m_x = 1.0f;
		}

		if (material->GetTextureMaterial() != nullptr)
		{
			descriptorWrites.push_back(material->GetTextureMaterial()->GetWriteDescriptor(3, descriptorSet));
			uboObject.samples.m_y = 1.0f;
		}

		if (material->GetTextureNormal() != nullptr)
		{
			descriptorWrites.push_back(material->GetTextureNormal()->GetWriteDescriptor(4, descriptorSet));
			uboObject.samples.m_z = 1.0f;
		}

		uboObject.diffuse = *material->GetColour();
		uboObject.surface = Vector3(material->GetMetallic(), material->GetRoughness(),
			(1.0f / 3.0f) * (static_cast<float>(material->GetIgnoreFog()) + 
			(2.0f * static_cast<float>(material->GetIgnoreLighting()))));

		m_uniformObject->Update(&uboObject);
		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		// Draws the object.
		VkDescriptorSet descriptors[] = {descriptorSet};
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipelineLayout(), 0, 1, descriptors, 0, nullptr);

		mesh->GetModel()->CmdRender(commandBuffer);
	}
}