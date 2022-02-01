#include "mesh.h"
#include "renderer.h"

void MeshRenderer::load(const Mesh &msh)
{
    mesh = &msh;
}

void MeshRenderer::init(Renderer &renderer)
{
    //vertex buffer
    vk::DeviceSize size = sizeof(Vertex) * mesh->vertices.size();

    vertex_buffer.init(size,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    Buffer vstaging = vertex_buffer.get_staging();
    vstaging.store(mesh->vertices.data());
    vstaging.copy(context.command_pool, vertex_buffer);
    vstaging.close();

    //index_buffer
    size = sizeof(mesh->indices[0]) * mesh->indices.size();

    index_buffer.init(size,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    Buffer istaging = index_buffer.get_staging();
    istaging.store(mesh->indices.data());
    istaging.copy(context.command_pool, index_buffer);
    istaging.close();

    //add to renderers list
    renderer.register_mesh(this);
}

void MeshRenderer::close()
{
    index_buffer.close();
    vertex_buffer.close();
}

void MeshRenderer::command_buffer(vk::CommandBuffer& command_buffer)
{
    vk::Buffer vertex_buffers[] = { vertex_buffer.buffer };
    vk::DeviceSize offsets[] = { 0 };
    command_buffer.bindVertexBuffers(0, 1, vertex_buffers, offsets);
    command_buffer.bindIndexBuffer(index_buffer.buffer, 0, vk::IndexType::eUint16);
    command_buffer.drawIndexed(static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
}