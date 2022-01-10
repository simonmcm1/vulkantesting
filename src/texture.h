#pragma once
#include <string>
#include "context.h"
#include "buffer.h"

#include <stb_image.h>

class Texture {
public:
	uint32_t width;
	uint32_t height;
	uint32_t channels;
	vk::Image image;
	vk::DeviceMemory image_memory;
	vk::ImageView image_view;
	vk::Sampler sampler;
	bool uploaded;

	static std::unique_ptr<Texture> load_image(Context &context, const std::string& path);

	void copy_from_buffer(Buffer& buffer);
	void upload();

	Texture(Context& ctx) : context(ctx), uploaded(false) {}
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
	void transition_layout(vk::ImageLayout layout);
};