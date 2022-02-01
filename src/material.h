#pragma once

#include "context.h"
#include "pipeline.h"
#include "asset_manager.h"

class MaterialType {
public:
	Pipeline pipeline;
	vk::DescriptorSetLayout descriptor_set_layout;

	MaterialType(Context& ctx) : context(ctx), pipeline(ctx) {}

	MaterialType(MaterialType& other) = delete;
	MaterialType(MaterialType&& other): context(other.context), pipeline(other.pipeline) {
		descriptor_set_layout = other.descriptor_set_layout;
		other.descriptor_set_layout = nullptr;
	}
	
	void init(vk::Extent2D viewport_extent, vk::RenderPass renderpass, vk::DescriptorSetLayout global_layout);
	void close_pipeline();
	void rebuild_pipeline(vk::Extent2D viewport_extent, vk::RenderPass renderpass, vk::DescriptorSetLayout global_layout);
	void close_layout();

private:
	Context& context;
	void init_descriptor_set_layout();
};


class Material {
public:
	Material(Context& ctx, MaterialType& type) : 
		context(ctx), 
		material_type(type),
		descriptor_set(nullptr) {}
	Material(Material& other) = delete;

	MaterialType& material_type;
	Pipeline &get_pipeline() const {
		return material_type.pipeline;
	}

	vk::DescriptorSet get_descriptor_set();
	void remove_descriptor_set();
	virtual void init_descriptor_set(vk::DescriptorPool& descriptor_pool, AssetManager& asset_manager) = 0;
protected:
	Context& context;
	vk::DescriptorSet descriptor_set;
};

class BasicMaterial : public Material {
public:
	BasicMaterial(Context& ctx, MaterialType& type) : Material(ctx, type) {}
	std::string albedo_texture;
	void init_descriptor_set(vk::DescriptorPool& descriptor_pool, AssetManager& asset_manager) override;
};

class MaterialManager {
public:
	MaterialManager(Context& ctx) : context(ctx) {}
	std::unordered_map<std::string, MaterialType> material_types;
	template<class T> std::unique_ptr<T> get_instance(const std::string& type) {
		auto& mt = material_types.at(type);
		return std::make_unique<T>(context, mt);
	}

	void init();
	void close_layouts();
	void close_pipelines();

private:
	Context& context;
};
