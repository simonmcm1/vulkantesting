#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>

#include <vulkan/vulkan.hpp>

std::unique_ptr<Texture> Texture::load_image(Context &context, const std::string& path)
{
	int w, h, channels;
	auto image = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
	if (image == nullptr) {
		throw std::runtime_error("failed to load image at " + path);
	}
	
	std::unique_ptr<Texture> tex = std::make_unique<Texture>(context);
	tex->pixels = image;
	tex->width = w;
	tex->height = h;
	tex->channels = channels;
	
	tex->layout = vk::ImageLayout::eUndefined;

	return tex;
}

void Texture::copy_from_buffer(Buffer& buffer)
{
	auto command = OneTimeSubmitCommand::create(context);

	vk::ImageSubresourceLayers subresource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	vk::BufferImageCopy region({},
		{},
		{},
		subresource,
		{ 0,0,0 },
		{ width,height,1 });

	command.buffer.copyBufferToImage(buffer.buffer, image, layout, 1, &region);
	command.execute();
}

void Texture::upload()
{
	Buffer staging(context);
	staging.init(width * height * channels,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	staging.store(pixels);

	vk::Extent3D extent(width, height, 1);
	vk::ImageCreateInfo create_info({},
		vk::ImageType::e2D,
		vk::Format::eR8G8B8A8Srgb,
		extent,
		1,
		1,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
		vk::SharingMode::eExclusive,
		{},
		{},
		layout);
	

	image = context.device.createImage(create_info);

	auto memory_requirements = context.device.getImageMemoryRequirements(image);
	vk::MemoryAllocateInfo allocate_info(memory_requirements.size,
		context.find_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	image_memory = context.device.allocateMemory(allocate_info);
	context.device.bindImageMemory(image, image_memory, 0);

	transition_layout(vk::ImageLayout::eTransferDstOptimal);
	copy_from_buffer(staging);
	transition_layout(vk::ImageLayout::eShaderReadOnlyOptimal);
	staging.close();

	//image view
	vk::ImageViewCreateInfo view_info({},
		image,
		vk::ImageViewType::e2D,
		vk::Format::eR8G8B8A8Srgb,
		vk::ComponentMapping{},
		vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	image_view = context.device.createImageView(view_info);

	//sampler
	vk::SamplerCreateInfo sampler_info({},
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		0.0f,
		true,
		1.0f,  //TODO: check device support
		false,
		vk::CompareOp::eAlways,
		0.0f,
		0.0f,
		vk::BorderColor::eIntOpaqueBlack,
		false);

	sampler = context.device.createSampler(sampler_info);

	uploaded = true;

}

void Texture::close()
{
	if (pixels != nullptr) {
		stbi_image_free(pixels);
		pixels = nullptr;
	}
	if (sampler != vk::Sampler(nullptr)) {
		context.device.destroySampler(sampler);
		sampler = nullptr;
	}
	if (image_view != vk::ImageView(nullptr)) {
		context.device.destroyImageView(image_view);
		image_view = nullptr;
	}
	if (image != vk::Image(nullptr)) {
		context.device.destroyImage(image);
		image = nullptr;
	}
	if (image_memory != vk::DeviceMemory(nullptr)) {
		context.device.freeMemory(image_memory);
		image_memory = nullptr;
	}
	
}

void Texture::transition_layout(vk::ImageLayout new_layout)
{
	auto command = OneTimeSubmitCommand::create(context);

	vk::AccessFlags src_access_mask;
	vk::AccessFlags dest_access_mask;
	vk::PipelineStageFlags src_stage;
	vk::PipelineStageFlags dest_stage;
	if (layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
		src_access_mask = {};
		dest_access_mask = vk::AccessFlagBits::eTransferWrite;
		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dest_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		src_access_mask = vk::AccessFlagBits::eTransferWrite;
		dest_access_mask == vk::AccessFlagBits::eShaderRead;
		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dest_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		throw std::invalid_argument("unhandled image layout transfer");
	}

	vk::ImageMemoryBarrier barrier(
		src_access_mask,
		dest_access_mask,
		layout,
		new_layout,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		image,
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	command.buffer.pipelineBarrier(src_stage, dest_stage, {}, 0, nullptr, 0, nullptr, 1, &barrier);

	command.execute();

	layout = new_layout;
}
