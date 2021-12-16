#pragma once

#include "context.h"
#include "log.h"

class Buffer {
public:
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    vk::DeviceSize size;

    Buffer(Context &ctx) : context(ctx) {}

    void init(vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags properties);
    void copy(vk::CommandPool &command_pool, Buffer &dest);
    Buffer get_staging();
    void store(const void *src);
    void close();

private:
    Context &context;
};