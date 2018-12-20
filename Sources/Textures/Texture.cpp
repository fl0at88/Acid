#include "Texture.hpp"

#include <cmath>
#include "Display/Display.hpp"
#include "Helpers/FileSystem.hpp"
#include "Files/Files.hpp"
#include "Renderer/Buffers/Buffer.hpp"
#include "Resources/Resources.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace acid
{
	static const std::string FALLBACK_PATH = "Undefined.png";
	static const float ANISOTROPY = 16.0f;

	std::shared_ptr<Texture> Texture::Resource(const std::string &filename)
	{
		if (filename.empty())
		{
			return nullptr;
		}

		auto resource = Resources::Get()->Find(filename);

		if (resource != nullptr)
		{
			return std::dynamic_pointer_cast<Texture>(resource);
		}

		auto result = std::make_shared<Texture>(filename);
		Resources::Get()->Add(std::dynamic_pointer_cast<IResource>(result));
		return result;
	}

	Texture::Texture(const std::string &filename, const VkFilter &filter, const VkSamplerAddressMode &addressMode, const bool &anisotropic, const bool &mipmap) :
		IResource(),
		IDescriptor(),
		m_filename(filename),
		m_filter(filter),
		m_addressMode(addressMode),
		m_anisotropic(anisotropic),
		m_mipLevels(1),
		m_samples(VK_SAMPLE_COUNT_1_BIT),
		m_imageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
		m_components(0),
		m_width(0),
		m_height(0),
		m_image(VK_NULL_HANDLE),
		m_deviceMemory(VK_NULL_HANDLE),
		m_imageView(VK_NULL_HANDLE),
		m_format(VK_FORMAT_R8G8B8A8_UNORM),
		m_imageInfo({})
	{
#if defined(ACID_VERBOSE)
		auto debugStart = Engine::GetTime();
#endif

		auto logicalDevice = Display::Get()->GetLogicalDevice();

		auto pixels = LoadPixels(m_filename, &m_width, &m_height, &m_components);

		m_mipLevels = mipmap ? GetMipLevels(m_width, m_height) : 1;

		Buffer bufferStaging = Buffer(m_width * m_height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void *data;
		vkMapMemory(logicalDevice, bufferStaging.GetBufferMemory(), 0, bufferStaging.GetSize(), 0, &data);
		memcpy(data, pixels, bufferStaging.GetSize());
		vkUnmapMemory(logicalDevice, bufferStaging.GetBufferMemory());

		CreateImage(m_image, m_deviceMemory, m_width, m_height, VK_IMAGE_TYPE_2D, m_samples, m_mipLevels, m_format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);
		TransitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels, 0, 1);
		CopyBufferToImage(bufferStaging.GetBuffer(), m_image, m_width, m_height, 0, 1);

		if (mipmap)
		{
			CreateMipmaps(m_image, m_width, m_height, m_imageLayout, m_mipLevels, 0, 1);
		}
		else
		{
			TransitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_imageLayout, m_mipLevels, 0, 1);
		}

		CreateImageSampler(m_sampler, m_filter, m_addressMode, m_anisotropic, m_mipLevels);
		CreateImageView(m_image, m_imageView, VK_IMAGE_VIEW_TYPE_2D, m_format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels, 0, 1);

		m_imageInfo = {
			.sampler = m_sampler,
			.imageView = m_imageView,
			.imageLayout = m_imageLayout
		};

		DeletePixels(pixels);
		m_filename = filename;

#if defined(ACID_VERBOSE)
		auto debugEnd = Engine::GetTime();
		Log::Out("Texture '%s' loaded in %ims\n", m_filename.c_str(), (debugEnd - debugStart).AsMilliseconds());
#endif
	}

	Texture::Texture(const uint32_t &width, const uint32_t &height, void *pixels, const VkFormat &format, const VkImageLayout &imageLayout, const VkImageUsageFlags &usage,
		const VkFilter &filter, const VkSamplerAddressMode &addressMode, const VkSampleCountFlagBits &samples, const bool &anisotropic, const bool &mipmap) :
		IResource(),
		IDescriptor(),
		m_filename(""),
		m_filter(filter),
		m_addressMode(addressMode),
		m_anisotropic(anisotropic),
		m_mipLevels(1),
		m_samples(samples),
		m_imageLayout(imageLayout),
		m_components(4),
		m_width(width),
		m_height(height),
		m_image(VK_NULL_HANDLE),
		m_deviceMemory(VK_NULL_HANDLE),
		m_imageView(VK_NULL_HANDLE),
		m_sampler(VK_NULL_HANDLE),
		m_format(format),
		m_imageInfo({})
	{
		auto logicalDevice = Display::Get()->GetLogicalDevice();

		m_mipLevels = mipmap ? GetMipLevels(m_width, m_height) : 1;

		CreateImage(m_image, m_deviceMemory, m_width, m_height, VK_IMAGE_TYPE_2D, m_samples, m_mipLevels, m_format, VK_IMAGE_TILING_OPTIMAL,
		            usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);

		if (pixels != nullptr || mipmap)
		{
			TransitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels, 0, 1);
		}

		if (pixels != nullptr)
		{
			Buffer bufferStaging = Buffer(m_width * m_height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			void *data;
			Display::CheckVk(vkMapMemory(logicalDevice, bufferStaging.GetBufferMemory(), 0, bufferStaging.GetSize(), 0, &data));
			memcpy(data, pixels, bufferStaging.GetSize());
			vkUnmapMemory(logicalDevice, bufferStaging.GetBufferMemory());

			CopyBufferToImage(bufferStaging.GetBuffer(), m_image, m_width, m_height, 0, 1);
		}

		if (mipmap)
		{
			CreateMipmaps(m_image, m_width, m_height, m_imageLayout, m_mipLevels, 0, 1);
		}
		else if (pixels != nullptr)
		{
			TransitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_imageLayout, m_mipLevels, 0, 1);
		}
		else
		{
			TransitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, m_imageLayout, m_mipLevels, 0, 1);
		}

		CreateImageSampler(m_sampler, m_filter, m_addressMode, m_anisotropic, m_mipLevels);
		CreateImageView(m_image, m_imageView, VK_IMAGE_VIEW_TYPE_2D, m_format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels, 0, 1);

		m_imageInfo = {
			.sampler = m_sampler,
			.imageView = m_imageView,
			.imageLayout = m_imageLayout
		};
	}

	Texture::~Texture()
	{
		auto logicalDevice = Display::Get()->GetLogicalDevice();

		vkDestroySampler(logicalDevice, m_sampler, nullptr);
		vkDestroyImageView(logicalDevice, m_imageView, nullptr);
		vkFreeMemory(logicalDevice, m_deviceMemory, nullptr);
		vkDestroyImage(logicalDevice, m_image, nullptr);
	}

	DescriptorType Texture::CreateDescriptor(const uint32_t &binding, const VkDescriptorType &descriptorType, const VkShaderStageFlags &stage, const uint32_t &count)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
			.binding = binding,
			.descriptorType = descriptorType,
			.descriptorCount = 1,
			.stageFlags = stage,
			.pImmutableSamplers = nullptr
		};
		VkDescriptorPoolSize descriptorPoolSize = {
			.type = descriptorType,
			.descriptorCount = count
		};
		return DescriptorType(binding, stage, descriptorSetLayoutBinding, descriptorPoolSize);
	}

	VkWriteDescriptorSet Texture::GetWriteDescriptor(const uint32_t &binding, const VkDescriptorType &descriptorType, const DescriptorSet &descriptorSet) const
	{
		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet.GetDescriptorSet(),
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = descriptorType,
			.pImageInfo = &m_imageInfo
		};
		return descriptorWrite;
	}

	uint8_t *Texture::GetPixels()
	{
		auto logicalDevice = Display::Get()->GetLogicalDevice();

		VkImage dstImage;
		VkDeviceMemory dstImageMemory;
		CopyImage(m_image, dstImage, dstImageMemory, m_width, m_height, false, 0, 1);

		VkImageSubresource imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.arrayLayer = 0
		};

		VkSubresourceLayout subresourceLayout;
		vkGetImageSubresourceLayout(logicalDevice, dstImage, &imageSubresource, &subresourceLayout);

		uint8_t *result = new uint8_t[subresourceLayout.size];

		void *data;
		vkMapMemory(logicalDevice, dstImageMemory, subresourceLayout.offset, subresourceLayout.size, 0, &data);
		memcpy(result, data, static_cast<size_t>(subresourceLayout.size));
		vkUnmapMemory(logicalDevice, dstImageMemory);

		vkFreeMemory(logicalDevice, dstImageMemory, nullptr);
		vkDestroyImage(logicalDevice, dstImage, nullptr);

		return result;
	}

	void Texture::SetPixels(uint8_t *pixels)
	{
		auto logicalDevice = Display::Get()->GetLogicalDevice();

		Buffer bufferStaging = Buffer(m_width * m_height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void *data;
		vkMapMemory(logicalDevice, bufferStaging.GetBufferMemory(), 0, bufferStaging.GetSize(), 0, &data);
		memcpy(data, pixels, bufferStaging.GetSize());
		vkUnmapMemory(logicalDevice, bufferStaging.GetBufferMemory());
	}

	uint8_t *Texture::LoadPixels(const std::string &filename, uint32_t *width, uint32_t *height, uint32_t *components)
	{
		auto fileLoaded = Files::Read(filename);

		if (!fileLoaded)
		{
			if (filename == FALLBACK_PATH)
			{
				return nullptr;
			}

			Log::Error("Texture could not be loaded: '%s'\n", filename.c_str());
			return LoadPixels(FALLBACK_PATH, width, height, components);
		}

		auto data = stbi_load_from_memory((uint8_t*)fileLoaded->data(), (uint32_t)fileLoaded->size(), (int32_t *)width, (int32_t *)height, (int32_t *)components, STBI_rgb_alpha);

		if (data == nullptr)
		{
			Log::Error("Unable to load texture: '%s'\n", filename.c_str());
		}

		return data;
	}

	uint8_t *Texture::LoadPixels(const std::string &filename, const std::string &fileSuffix, const std::vector<std::string> &fileSides, uint32_t *width, uint32_t *height, uint32_t *components)
	{
		stbi_uc *pixels = nullptr;
		stbi_uc *offset = nullptr;

		for (auto &side : fileSides)
		{
			std::string filenameSide = std::string(filename).append("/").append(side).append(fileSuffix);
			auto pixelsSide = LoadPixels(filenameSide, width, height, components);
			int32_t sizeSide = *width * *height * 4;

			if (pixels == nullptr)
			{
				pixels = (stbi_uc *) malloc(sizeSide * fileSides.size());
				offset = pixels;
			}

			memcpy(offset, pixelsSide, sizeSide);
			offset += sizeSide;
			free(pixelsSide);
		}

		return pixels;
	}

	bool Texture::WritePixels(const std::string &filename, const void *data, const int32_t &width, const int32_t &height, const int32_t &components)
	{
		int32_t result = stbi_write_png(filename.c_str(), width, height, components, data, width * components);
		return result == 1;
	}

	void Texture::DeletePixels(uint8_t *pixels)
	{
		stbi_image_free(pixels);
	}

	uint32_t Texture::GetMipLevels(const uint32_t &width, const uint32_t &height)
	{
		return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);
	}

	void Texture::CreateImage(VkImage &image, VkDeviceMemory &imageMemory, const uint32_t &width, const uint32_t &height, const VkImageType &type, const VkSampleCountFlagBits &samples,
		const uint32_t &mipLevels, const VkFormat &format, const VkImageTiling &tiling, const VkImageUsageFlags &usage, const VkMemoryPropertyFlags &properties, const uint32_t &arrayLayers)
	{
		auto logicalDevice = Display::Get()->GetLogicalDevice();

		VkImageCreateInfo imageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = static_cast<VkImageCreateFlags>(arrayLayers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0),
			.imageType = type,
			.format = format,
			.extent = {
				.width = width,
				.height = height,
				.depth = 1
			},
			.mipLevels = mipLevels,
			.arrayLayers = arrayLayers,
			.samples = samples,
			.tiling = tiling,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		Display::CheckVk(vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image));

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(logicalDevice, image, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memoryRequirements.size,
			.memoryTypeIndex = Buffer::FindMemoryType(memoryRequirements.memoryTypeBits, properties)
		};
		Display::CheckVk(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &imageMemory));

		Display::CheckVk(vkBindImageMemory(logicalDevice, image, imageMemory, 0));
	}

	bool Texture::HasStencilComponent(const VkFormat &format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void Texture::TransitionImageLayout(const VkImage &image, const VkFormat &format, const VkImageLayout &srcImageLayout, const VkImageLayout &dstImageLayout, const uint32_t &mipLevels, const uint32_t &baseArrayLayer, const uint32_t &layerCount)
	{
		CommandBuffer commandBuffer = CommandBuffer();

		VkImageMemoryBarrier imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.oldLayout = srcImageLayout,
			.newLayout = dstImageLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = baseArrayLayer,
				.layerCount = layerCount
			}
		};

		if (dstImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format))
			{
				imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		VkPipelineStageFlags srcStageMask;
		VkPipelineStageFlags dstStageMask;

		if (srcImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (srcImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (srcImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (srcImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (srcImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStageMask = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		else
		{
			assert(false && "Unsupported image layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer.GetCommandBuffer(), srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

		commandBuffer.End();
		commandBuffer.Submit();
	}

	void Texture::CopyBufferToImage(const VkBuffer &buffer, const VkImage &image, const uint32_t &width, const uint32_t &height, const uint32_t &baseArrayLayer, const uint32_t &layerCount)
	{
		CommandBuffer commandBuffer = CommandBuffer();

		VkBufferImageCopy region = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = baseArrayLayer,
				.layerCount = layerCount
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = {
				.width = width,
				.height = height,
				.depth = 1
			}
		};
		vkCmdCopyBufferToImage(commandBuffer.GetCommandBuffer(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		commandBuffer.End();
		commandBuffer.Submit();
	}

	void Texture::CreateMipmaps(const VkImage &image, const uint32_t &width, const uint32_t &height, const VkImageLayout &dstImageLayout, const uint32_t &mipLevels, const uint32_t &baseArrayLayer, const uint32_t &layerCount)
	{
		CommandBuffer commandBuffer = CommandBuffer();

		int32_t mipWidth = width;
		int32_t mipHeight = height;

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			VkImageMemoryBarrier barrier0 = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = i - 1,
					.levelCount = 1,
					.baseArrayLayer = baseArrayLayer,
					.layerCount = layerCount,
				}
			};
			vkCmdPipelineBarrier(commandBuffer.GetCommandBuffer(),
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier0);

			VkImageBlit imageBlit = {
				.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i - 1,
					.baseArrayLayer = baseArrayLayer,
					.layerCount = layerCount
				},
				.srcOffsets = {
					{0, 0, 0},
					{mipWidth, mipHeight, 1}
				},
				.dstSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i,
					.baseArrayLayer = baseArrayLayer,
					.layerCount = layerCount
				},
				.dstOffsets = {
					{0, 0, 0},
					{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}
				}
			};
			vkCmdBlitImage(commandBuffer.GetCommandBuffer(),
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &imageBlit,
				VK_FILTER_LINEAR);

			VkImageMemoryBarrier barrier1 = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = i - 1,
					.levelCount = 1,
					.baseArrayLayer = baseArrayLayer,
					.layerCount = layerCount,
				}
			};
			vkCmdPipelineBarrier(commandBuffer.GetCommandBuffer(),
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier1);

			if (mipWidth > 1)
			{
				mipWidth /= 2;
			}

			if (mipHeight > 1)
			{
				mipHeight /= 2;
			}
		}

		VkImageMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = dstImageLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = mipLevels - 1,
				.levelCount = 1,
				.baseArrayLayer = baseArrayLayer,
				.layerCount = layerCount,
			}
		};
		vkCmdPipelineBarrier(commandBuffer.GetCommandBuffer(),
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		commandBuffer.End();
		commandBuffer.Submit();
	}

	void Texture::CreateImageSampler(VkSampler &sampler, const VkFilter &filter, const VkSamplerAddressMode &addressMode, const bool &anisotropic, const uint32_t &mipLevels)
	{
		auto logicalDevice = Display::Get()->GetLogicalDevice();

		VkSamplerCreateInfo samplerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = filter,
			.minFilter = filter,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = addressMode,
			.addressModeV = addressMode,
			.addressModeW = addressMode,
			.mipLodBias = 0.0f,
			.anisotropyEnable = anisotropic,
			.maxAnisotropy = std::min(ANISOTROPY, Display::Get()->GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy),
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.0f,
			.maxLod = static_cast<float>(mipLevels),
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE
		};
		Display::CheckVk(vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr, &sampler));
	}

	void Texture::CreateImageView(const VkImage &image, VkImageView &imageView, const VkImageViewType &type, const VkFormat &format, const VkImageAspectFlags &imageAspect, const uint32_t &mipLevels, const uint32_t &baseArrayLayer, const uint32_t &layerCount)
	{
		auto logicalDevice = Display::Get()->GetLogicalDevice();

		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = type,
			.format = format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = imageAspect,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = baseArrayLayer,
				.layerCount = layerCount
			}
		};
		Display::CheckVk(vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView));
	}

	bool Texture::CopyImage(const VkImage &srcImage, VkImage &dstImage, VkDeviceMemory &dstImageMemory, const uint32_t &width, const uint32_t &height, const bool &srcSwapchain, const uint32_t &baseArrayLayer, const uint32_t &layerCount)
	{
		auto physicalDevice = Display::Get()->GetPhysicalDevice();
		auto surfaceFormat = Display::Get()->GetSurfaceFormat();

		// Checks blit swapchain support.
		bool supportsBlit = true;
		VkFormatProperties formatProperties;

		// Check if the device supports blitting from optimal images (the swapchain images are in optimal format).
		vkGetPhysicalDeviceFormatProperties(physicalDevice, surfaceFormat.format, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
		{
			supportsBlit = false;
		}

		// Check if the device supports blitting to linear images.
		vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);

		if (!(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
		{
			supportsBlit = false;
		}

		CreateImage(dstImage, dstImageMemory, width, height, VK_IMAGE_TYPE_2D, VK_SAMPLE_COUNT_1_BIT, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);

		// Do the actual blit from the swapchain image to our host visible destination image.
		CommandBuffer commandBuffer = CommandBuffer();

		// Transition destination image to transfer destination layout.
		InsertImageMemoryBarrier(
			commandBuffer.GetCommandBuffer(),
			dstImage,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

		// Transition swapchain image from present to transfer source layout
		if (srcSwapchain)
		{
			InsertImageMemoryBarrier(
				commandBuffer.GetCommandBuffer(),
				srcImage,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, baseArrayLayer, layerCount});
		}

		// If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB).
		if (supportsBlit)
		{
			// Define the region to blit (we will blit the whole swapchain image).
			VkOffset3D blitSize = {
				.x = static_cast<int32_t>(width),
				.y = static_cast<int32_t>(height),
				.z = 1
			};
			VkImageBlit imageBlitRegion = {
				.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseArrayLayer = baseArrayLayer,
					.layerCount = 1
				},
				.srcOffsets = {
					{ 0, 0, 0 },
					blitSize
				},
				.dstSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
				.dstOffsets = {
					{ 0, 0, 0 },
					blitSize
				}
			};
			vkCmdBlitImage(commandBuffer.GetCommandBuffer(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VK_FILTER_NEAREST);
		}
		else
		{
			// Otherwise use image copy (requires us to manually flip components).
			VkImageCopy imageCopyRegion = {
				.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseArrayLayer = baseArrayLayer,
					.layerCount = 1
				},
				.dstSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
				.extent = {
					.width = width,
					.height = height,
					.depth = 1,
				}
			};
			vkCmdCopyImage(commandBuffer.GetCommandBuffer(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
		}

		// Transition destination image to general layout, which is the required layout for mapping the image memory later on.
		InsertImageMemoryBarrier(
			commandBuffer.GetCommandBuffer(),
			dstImage,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

		// Transition back the swap chain image after the blit is done.
		if (srcSwapchain)
		{
			InsertImageMemoryBarrier(
				commandBuffer.GetCommandBuffer(),
				srcImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, baseArrayLayer, layerCount});
		}

		commandBuffer.End();
		commandBuffer.Submit();

		return supportsBlit;
	}

	void Texture::InsertImageMemoryBarrier(const VkCommandBuffer &cmdbuffer, const VkImage &image, const VkAccessFlags &srcAccessMask, const VkAccessFlags &dstAccessMask, const VkImageLayout &oldImageLayout, const VkImageLayout &newImageLayout, const VkPipelineStageFlags &srcStageMask, const VkPipelineStageFlags &dstStageMask, const VkImageSubresourceRange &subresourceRange)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = srcAccessMask,
			.dstAccessMask = dstAccessMask,
			.oldLayout = oldImageLayout,
			.newLayout = newImageLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = subresourceRange
		};
		vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
}
