#include "buffer.h"
#include "log.h"

void Buffer::init(vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags properties) {

    this->size = size;

    vk::BufferCreateInfo buffer_info({},
                                     size,
                                     usage_flags,
                                     vk::SharingMode::eExclusive,
                                     {}, {});
    buffer = context.device.createBuffer(buffer_info);

    auto memory_reqs = context.device.getBufferMemoryRequirements(buffer);
    auto memory_type = context.find_memory_type(memory_reqs.memoryTypeBits, properties);
    vk::MemoryAllocateInfo alloc_info(memory_reqs.size, memory_type);

    memory = context.device.allocateMemory(alloc_info);
    context.device.bindBufferMemory(buffer, memory, 0);
}

void Buffer::copy(vk::CommandPool &command_pool, Buffer &dest) {
    vk::CommandBufferAllocateInfo alloc_info(command_pool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer command_buffer = context.device.allocateCommandBuffers(alloc_info)[0];

    vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);
    vk::BufferCopy copy_info(0, 0, size);
    command_buffer.copyBuffer(buffer, dest.buffer, 1, &copy_info);
    command_buffer.end();

    vk::SubmitInfo submit_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    context.graphics_queue.submit(1, &submit_info, nullptr);
    context.graphics_queue.waitIdle();

    context.device.freeCommandBuffers(command_pool, 1, &command_buffer);
}

Buffer Buffer::get_staging() {
    Buffer staging(context);
    staging.init(size,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    return std::move(staging);
}

void Buffer::store(const void* src) {
    void *data = context.device.mapMemory(memory, 0, size);
    memcpy(data, src, (size_t)size);
    context.device.unmapMemory(memory);
}

void Buffer::close() {
    if (buffer != vk::Buffer(nullptr)) {
        context.device.destroyBuffer(buffer);
        context.device.freeMemory(memory);
    }    
}