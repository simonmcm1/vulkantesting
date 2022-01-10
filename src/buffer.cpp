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
    auto command = OneTimeSubmitCommand::create(context);
    vk::BufferCopy copy_info(0, 0, size);
    command.buffer.copyBuffer(buffer, dest.buffer, 1, &copy_info);
    command.execute();
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