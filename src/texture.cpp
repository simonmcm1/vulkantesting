#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>

#include <vulkan/vulkan.hpp>

std::unique_ptr<Texture> Texture::load_image(Context& context, const std::string& path, ColorSpace color_space)
{
	int w, h, channels;
	auto image = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
	if (image == nullptr) {
		throw std::runtime_error("failed to load image at " + path);
	}

	std::cout << "loaded " << path << ", " << w << "x" << h << " " << channels << " chanels" << std::endl;

	std::unique_ptr<Texture> tex = std::make_unique<Texture>(context);
	tex->pixels = image;
	tex->width = w;
	tex->height = h;
	tex->channels = 4;
	if (color_space == SRGB) {
		tex->format = vk::Format::eR8G8B8A8Srgb;
	}
	else if (color_space == LINEAR) {
		tex->format = vk::Format::eR8G8B8A8Snorm;
	}
	
	tex->aspect = vk::ImageAspectFlagBits::eColor;
	tex->tiling = vk::ImageTiling::eOptimal;
	tex->usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
	tex->memory_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;

	tex->mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex->width, tex->height)))) + 1;

	return tex;
}

vk::Format Texture::get_supported_format(Context& context,
	const std::vector<vk::Format>& candidates,
	vk::ImageTiling tiling,
	vk::FormatFeatureFlags features)
{

	for (const auto& candidate : candidates) {
		auto props = context.physical_device.getFormatProperties(candidate);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return candidate;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return candidate;
		}
	}
	throw std::runtime_error("no supported texture format");
}

bool Texture::has_stencil(vk::Format format)
{
	const vk::Format stencil_formats[] {
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD24UnormS8Uint,
		vk::Format::eD16UnormS8Uint,
		vk::Format::eS8Uint
	};
	for (const auto stencil : stencil_formats) {
		if (format == stencil) {
			return true;
		}
	}
	return false;
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

void Texture::init()
{
	if (format == vk::Format::eUndefined) {
		throw std::runtime_error("tried to init image with undefined format");
	}

    vk::Extent3D extent(width, height, 1);

	vk::ImageCreateInfo create_info({},
		vk::ImageType::e2D,
		format,
		extent,
		mip_levels,
		1,
		vk::SampleCountFlagBits::e1,
		tiling,
		usage,
		vk::SharingMode::eExclusive,
		{},
		{},
		layout);

	image = context.device.createImage(create_info);

	auto memory_requirements = context.device.getImageMemoryRequirements(image);
	vk::MemoryAllocateInfo allocate_info(memory_requirements.size,
		context.find_memory_type(memory_requirements.memoryTypeBits, memory_flags));

	image_memory = context.device.allocateMemory(allocate_info);
	context.device.bindImageMemory(image, image_memory, 0);

	//image view
	vk::ImageViewCreateInfo view_info({},
		image,
		vk::ImageViewType::e2D,
		format,
		vk::ComponentMapping{},
		vk::ImageSubresourceRange{ aspect, 0, mip_levels, 0, 1 });

	image_view = context.device.createImageView(view_info);

}

void Texture::upload(bool generate_mipmaps)
{

	Buffer staging(context);
	staging.init(width * height * channels,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	staging.store(pixels);

	transition_layout(vk::ImageLayout::eTransferDstOptimal);
	copy_from_buffer(staging);

	staging.close();

	if (generate_mipmaps) {
		//create_mipmaps also transitions layout to ShaderReadOnlyOptimal
		create_mipmaps();
	} else {
		transition_layout(vk::ImageLayout::eShaderReadOnlyOptimal);
	}
	

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
		static_cast<float>(mip_levels),
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

void Texture::create_mipmaps()
{
	auto command = OneTimeSubmitCommand::create(context);

	vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
	vk::ImageMemoryBarrier barrier(
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eTransferRead,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eTransferSrcOptimal,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		image,
		subresource_range
	);

	int32_t mip_width = width;
	int32_t mip_height = height;

	for (uint32_t i = 1; i < mip_levels; i++) {
		uint32_t next_mip_width = mip_width > 1 ? mip_width / 2 : 1;
		uint32_t next_mip_height = mip_height > 1 ? mip_height / 2 : 1;

		//we are going to read from the level below, transfer that level from dst to src
		//this level is already a dest
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal,
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal,
		barrier.subresourceRange.baseMipLevel = i - 1;

		command.buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			0, nullptr,
			0, nullptr,
			1, &barrier);

		std::array<vk::Offset3D, 2> src_offset = {
			vk::Offset3D{0,0,0},
			vk::Offset3D{mip_width, mip_height, 1}
		};
		std::array<vk::Offset3D, 2> dest_offset = {
			vk::Offset3D{0,0,0},
			vk::Offset3D{static_cast<int32_t>(next_mip_width), static_cast<int32_t>(next_mip_height), 1}
		};
		vk::ImageBlit blit(
			vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, i - 1, 0, 1 },
			src_offset,
			vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, i, 0, 1 },
			dest_offset
		);
		command.buffer.blitImage(
			image, vk::ImageLayout::eTransferSrcOptimal,
			image, vk::ImageLayout::eTransferDstOptimal,
			1, &blit,
			vk::Filter::eLinear
		);

		//transfer the level below us to the final desired layout
		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.subresourceRange.baseMipLevel = i - 1;

		command.buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			{},
			0, nullptr,
			0, nullptr,
			1, &barrier);

		mip_width = next_mip_width;
		mip_height = next_mip_height;
	}

	//transfer the final level that didn't get done in the loop
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	barrier.subresourceRange.baseMipLevel = mip_levels - 1;

	command.buffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eFragmentShader,
		{},
		0, nullptr,
		0, nullptr,
		1, &barrier);

	command.execute();
	layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

void Texture::transition_layout(vk::ImageLayout new_layout)
{
	auto command = OneTimeSubmitCommand::create(context);

	vk::AccessFlags src_access_mask;
	vk::AccessFlags dest_access_mask;
	vk::PipelineStageFlags src_stage;
	vk::PipelineStageFlags dest_stage;

	vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1);

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
	else if (layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		src_access_mask = {};
		dest_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dest_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;

		if (has_stencil(format)) {
			subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		} else {
			subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
		}
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
		subresource_range);
		

	command.buffer.pipelineBarrier(src_stage, dest_stage, {}, 0, nullptr, 0, nullptr, 1, &barrier);

	command.execute();

	layout = new_layout;
}
