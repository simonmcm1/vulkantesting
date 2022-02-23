#include "material.h"


void MaterialType::init_descriptor_set_layout() {
	std::cout << "initing mattype " << name << std::endl;
	if (name == "basic")
	{
		vk::DescriptorSetLayoutBinding sampler_layout(0,
			vk::DescriptorType::eCombinedImageSampler,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr);
		std::array<vk::DescriptorSetLayoutBinding, 1> bindings{ sampler_layout };
		vk::DescriptorSetLayoutCreateInfo create_info({}, static_cast<uint32_t>(bindings.size()), bindings.data());
		descriptor_set_layout = context.device.createDescriptorSetLayout(create_info);
	}
	else if (name == "colored")
	{
		vk::DescriptorSetLayoutBinding ubo_layout(0,
			vk::DescriptorType::eUniformBuffer,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr);
		std::array<vk::DescriptorSetLayoutBinding, 1> bindings{ ubo_layout };
		vk::DescriptorSetLayoutCreateInfo create_info({}, static_cast<uint32_t>(bindings.size()), bindings.data());
		descriptor_set_layout = context.device.createDescriptorSetLayout(create_info);
	}
	else if (name == "standard") {
		vk::DescriptorSetLayoutBinding ubo_layout(0,
			vk::DescriptorType::eUniformBuffer,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr);
		vk::DescriptorSetLayoutBinding alb_sampler_layout(1,
			vk::DescriptorType::eCombinedImageSampler,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr);
		vk::DescriptorSetLayoutBinding norm_sampler_layout(2,
			vk::DescriptorType::eCombinedImageSampler,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr);
		vk::DescriptorSetLayoutBinding maps_sampler_layout(3,
			vk::DescriptorType::eCombinedImageSampler,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr);
		std::array<vk::DescriptorSetLayoutBinding, 4> bindings{ ubo_layout, alb_sampler_layout, norm_sampler_layout, maps_sampler_layout };
		vk::DescriptorSetLayoutCreateInfo create_info({}, static_cast<uint32_t>(bindings.size()), bindings.data());
		descriptor_set_layout = context.device.createDescriptorSetLayout(create_info);

	}
	else {
		throw std::runtime_error("unimplemented MaterialType: " + name);
	}
}

void MaterialType::init(vk::Extent2D extent, vk::RenderPass renderpass, vk::DescriptorSetLayout global_layout) {
	init_descriptor_set_layout();
	rebuild_pipeline(extent, renderpass, global_layout);
}

void MaterialType::rebuild_pipeline(vk::Extent2D extent, vk::RenderPass renderpass, vk::DescriptorSetLayout global_layout)
{
	if (descriptor_set_layout == vk::DescriptorSetLayout(nullptr)) {
		pipeline.init(extent, renderpass, { global_layout });
	}
	else {
		pipeline.init(extent, renderpass, { global_layout, descriptor_set_layout });
	}
}

void MaterialType::close_layout()
{
	if (descriptor_set_layout != vk::DescriptorSetLayout(nullptr)) {
		context.device.destroyDescriptorSetLayout(descriptor_set_layout);
	}
}

void MaterialType::close_pipeline() {
	pipeline.close();
}

void MaterialManager::init()
{
	MaterialType basic_material(context, "basic");
	material_types.insert(std::make_pair("basic", std::move(basic_material)));

	MaterialType colored_material(context, "colored");
	material_types.insert(std::make_pair("colored", std::move(colored_material)));

	MaterialType standard_material(context, "standard");
	material_types.insert(std::make_pair("standard", std::move(standard_material)));
}

void MaterialManager::close_layouts()
{
	for (auto& type : material_types) {
		type.second.close_layout();
	}
}

void MaterialManager::close_pipelines()
{
	for (auto& type : material_types) {
		type.second.close_pipeline();
	}
}

vk::DescriptorSet Material::get_descriptor_set()
{
	return descriptor_set;
}

void Material::remove_descriptor_set()
{
	descriptor_set = nullptr;
}

void BasicMaterial::init_descriptor_set(vk::DescriptorPool& descriptor_pool, AssetManager& asset_manager)
{
	vk::DescriptorSetAllocateInfo allocate_info(descriptor_pool, 1, &material_type.descriptor_set_layout);

	descriptor_set = context.device.allocateDescriptorSets(allocate_info).at(0);

	auto img = asset_manager.get_texture(albedo_texture);
	vk::DescriptorImageInfo image_info(img->sampler, img->image_view, img->get_layout());

	std::array<vk::WriteDescriptorSet, 1> writes{
			vk::WriteDescriptorSet(descriptor_set,
									0,
									0,
									1,
									vk::DescriptorType::eCombinedImageSampler,
									&image_info,
									nullptr,
									nullptr)
	};


	context.device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

}

void ColoredMaterial::init_descriptor_set(vk::DescriptorPool& descriptor_pool, AssetManager& asset_manager)
{
	vk::DescriptorSetAllocateInfo allocate_info(descriptor_pool, 1, &material_type.descriptor_set_layout);
	descriptor_set = context.device.allocateDescriptorSets(allocate_info).at(0);

	//TODO: use dynamic here
	vk::DescriptorBufferInfo buffer_info(uniform_buffer.buffer, 0, sizeof(Uniforms));

	std::array<vk::WriteDescriptorSet, 1> writes{
		vk::WriteDescriptorSet(descriptor_set,
							 0,
							 0,
							 1,
							 vk::DescriptorType::eUniformBuffer,
							 nullptr,
							 &buffer_info,
							 nullptr)
	};

	context.device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void ColoredMaterial::update_if_dirty() {
	if (dirty) {
		uniform_buffer.store(&uniforms);
		dirty = false;
	}
}

void ColoredMaterial::set_color(glm::vec4 color) {
	uniforms.color = color;
	dirty = true;
}

void ColoredMaterial::init() {
	uniform_buffer.init(sizeof(Uniforms),
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void ColoredMaterial::close() {
	uniform_buffer.close();
}

void StandardMaterial::update_if_dirty()
{
	if (dirty) {
		uniform_buffer.store(&uniforms);
		dirty = false;
	}
}

void StandardMaterial::init()
{
	uniform_buffer.init(sizeof(Uniforms),
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void StandardMaterial::close()
{
	uniform_buffer.close();
}

void StandardMaterial::init_descriptor_set(vk::DescriptorPool& descriptor_pool, AssetManager& asset_manager)
{
	vk::DescriptorSetAllocateInfo allocate_info(descriptor_pool, 1, &material_type.descriptor_set_layout);

	descriptor_set = context.device.allocateDescriptorSets(allocate_info).at(0);

	auto albedo_tex = asset_manager.get_texture(albedo_texture);
	vk::DescriptorImageInfo albedo_image_info(albedo_tex->sampler, albedo_tex->image_view, albedo_tex->get_layout());
	auto normal_tex = asset_manager.get_texture(normal_map);
	vk::DescriptorImageInfo normal_image_info(normal_tex->sampler, normal_tex->image_view, normal_tex->get_layout());
	auto props_tex = asset_manager.get_texture(pbr_map);
	vk::DescriptorImageInfo props_image_info(props_tex->sampler, props_tex->image_view, props_tex->get_layout());

	//TODO: use dynamic here
	vk::DescriptorBufferInfo buffer_info(uniform_buffer.buffer, 0, sizeof(Uniforms));

	std::array<vk::WriteDescriptorSet, 4> writes{
			vk::WriteDescriptorSet(descriptor_set,
									 0,
									 0,
									 1,
									 vk::DescriptorType::eUniformBuffer,
									 nullptr,
									 &buffer_info,
									 nullptr),
			vk::WriteDescriptorSet(descriptor_set,
									1,
									0,
									1,
									vk::DescriptorType::eCombinedImageSampler,
									&albedo_image_info,
									nullptr,
									nullptr),
			vk::WriteDescriptorSet(descriptor_set,
									2,
									0,
									1,
									vk::DescriptorType::eCombinedImageSampler,
									&normal_image_info,
									nullptr,
									nullptr),
			vk::WriteDescriptorSet(descriptor_set,
									3,
									0,
									1,
									vk::DescriptorType::eCombinedImageSampler,
									&props_image_info,
									nullptr,
									nullptr)
	};


	context.device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
