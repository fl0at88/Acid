#include "MeshRender.hpp"

#include "Meshes/Mesh.hpp"
#include "Animations/MeshAnimated.hpp"
#include "Entities/MaterialEntity.hpp"

namespace Flounder
{
	const float MAX_JOINTS = 50;
	const float MAX_WEIGHTS = 3;

	MeshRender::MeshRender() :
		Component(),
		m_uniformObject(nullptr),
		m_descriptorSet(nullptr)
	{
	}

	MeshRender::~MeshRender()
	{
		delete m_uniformObject;
		delete m_descriptorSet;
	}

	void MeshRender::Update()
	{
		auto materialEntity = GetGameObject()->GetComponent<MaterialEntity>();

		if (materialEntity == nullptr)
		{
			return;
		}

		auto material = materialEntity->GetMaterial();

		// Updates uniforms.
		if (m_uniformObject == nullptr)
		{
			m_uniformObject = new UniformBuffer((VkDeviceSize) material->GetPipeline()->GetShaderProgram()->GetUniformBlock("UniformObject")->m_size);
		}

		/*std::vector<Matrix4> jointTransforms = {};
		auto meshAnimated = GetGameObject()->GetComponent<MeshAnimated>();

		if (meshAnimated != nullptr)
		{
			auto jointMatrices = meshAnimated->GetJointTransforms();

			for (unsigned int i = 0; i < jointMatrices.size(); i++)
			{
				jointTransforms.push_back(*jointMatrices.at(i));

				if (jointTransforms.size() >= MAX_JOINTS)
				{
					break;
				}
			}
		}*/

		struct
		{
			//	Matrix4 jointTransforms[MAX_JOINTS];
			Matrix4 transform;
			Vector4 baseColor;
			float metallic;
			float roughness;
			float ignoreFog;
			float ignoreLighting;
		} uboObject;
		uboObject.transform = GetGameObject()->GetTransform()->GetWorldMatrix();
		uboObject.baseColor = *materialEntity->GetBaseColor();
		uboObject.metallic = materialEntity->GetMetallic();
		uboObject.roughness = materialEntity->GetRoughness();
		uboObject.ignoreFog = static_cast<float>(materialEntity->IsIgnoringFog());
		uboObject.ignoreLighting = static_cast<float>(materialEntity->IsIgnoringLighting());
		m_uniformObject->Update(&uboObject);

		//m_uniformObject->UpdateMap("UniformObject", material->GetPipeline()->GetShaderProgram(), {
		//	{"jointTransforms", jointTransforms.data()},
		//	{"transform", GetGameObject()->GetTransform()->GetWorldMatrix().m_elements},
		//	{"baseColor", materialDefault->GetBaseColor()->m_elements},
		//	{"metallic", materialDefault->GetMetallic()},
		//	{"roughness", materialDefault->GetRoughness()},
		//	{"ignoreFog", static_cast<float>(materialDefault->IsIgnoringFog())},
		//	{"ignoreLighting", static_cast<float>(materialDefault->IsIgnoringLighting())}
		//});
	}

	void MeshRender::Load(LoadedValue *value)
	{
	}

	void MeshRender::Write(LoadedValue *value)
	{
	}

	void MeshRender::CmdRender(const VkCommandBuffer &commandBuffer, UniformBuffer *uniformScene)
	{
		// Checks if the mesh is in view.
		/*auto rigidbody = GetGameObject()->GetComponent<Rigidbody>();

		if (rigidbody != nullptr && rigidbody->GetCollider() != nullptr)
		{
			if (!rigidbody->GetCollider()->InFrustum(*Scenes::Get()->GetCamera()->GetViewFrustum()))
			{
				return;
			}
		}*/

		// Gets required components.
		auto mesh = GetGameObject()->GetComponent<Mesh>();
		auto materialDefault = GetGameObject()->GetComponent<MaterialEntity>();

		if (mesh == nullptr || mesh->GetModel() == nullptr || materialDefault == nullptr || materialDefault->GetMaterial() == nullptr)
		{
			return;
		}

		auto material = materialDefault->GetMaterial();

		// Updates descriptors.
		if (m_descriptorSet == nullptr)
		{
			m_descriptorSet = new DescriptorSet(*material->GetPipeline());
		}

		m_descriptorSet->UpdateMap({
			{"UboScene",        uniformScene},
			{"UboObject",       m_uniformObject},
			{"samplerDiffuse",  materialDefault->GetDiffuseTexture()},
			{"samplerMaterial", materialDefault->GetMaterialTexture()},
			{"samplerNormal",   materialDefault->GetNormalTexture()}
		});

		// Draws the object.
		material->CmdRender(commandBuffer, mesh->GetModel(), m_descriptorSet);
	}
}
