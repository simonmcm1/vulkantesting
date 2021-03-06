#pragma once
#include <string>
#include "context.h"
#include "buffer.h"

#include <stb_image.h>

enum ColorSpace {
	SRGB,
	LINEAR
};

class Texture {
public:
	uint32_t width;
	uint32_t height;
	uint32_t channels;
	uint32_t mip_levels;
	vk::Image image;
	vk::DeviceMemory image_memory;
	vk::ImageView image_view;
	vk::Sampler sampler;
	vk::Format format;
	vk::ImageAspectFlags aspect;
	vk::ImageTiling tiling;
	vk::ImageUsageFlags usage;
	vk::MemoryPropertyFlagBits memory_flags;

	bool uploaded;

	static std::unique_ptr<Texture> load_image(Context &context, const std::string& path, ColorSpace color_space);
	static vk::Format get_supported_format(Context& context,
		const std::vector<vk::Format>& candidates,
		vk::ImageTiling tiling,
		vk::FormatFeatureFlags features);
	static bool has_stencil(vk::Format format);
	void init();
	void copy_from_buffer(Buffer& buffer);
	void upload(bool generate_mipmaps);
	void transition_layout(vk::ImageLayout layout);

	Texture(Context& ctx) : 
		context(ctx), 
		uploaded(false), 
		format(vk::Format::eUndefined),
		aspect(vk::ImageAspectFlagBits::eColor),
        layout(vk::ImageLayout::eUndefined),
        pixels(nullptr) {}
	
	~Texture() {
		close();
	}
	Texture(Texture& other) = delete;
    
	vk::ImageLayout get_layout() {
		return layout;
	}

	void close();

private:
	Context& context;
	stbi_uc* pixels;
	
	vk::ImageLayout layout;

	void create_mipmaps();
};