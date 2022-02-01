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

void BasicMaterial::init_descriptor_set(vk::DescriptorPool &descriptor_pool, AssetManager &asset_manager)
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

    vk::DeviceSize size = sizeof(Uniforms);
    uniform_buffer.init(size,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    //TODO: use dynamic here
    vk::DescriptorBufferInfo buffer_info(uniform_buffer.buffer, 0, size);

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
    Uniforms data{};
    data.color = { 1.0f, 0.0f, 0.0f, 1.0f };
    uniform_buffer.store(&data);
    context.device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);


}
