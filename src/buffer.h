#pragma once

#include "context.h"
#include "log.h"

class Buffer {
public:
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    vk::DeviceSize size;

    Buffer(Context& ctx) : context(ctx), buffer(nullptr), memory(nullptr), size(0) {}

    Buffer(const Buffer& other) = delete;
    Buffer(Buffer&& other) noexcept : Buffer(other.context) {
        buffer = other.buffer;
        memory = other.memory;
        size = other.size;
        other.buffer = nullptr;
        other.memory = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (&other != this) {
            buffer = other.buffer;
            memory = other.memory;
            size = other.size;
            other.buffer = nullptr;
            other.memory = nullptr;
            other.size = 0;
        }

        return *this;
    }



    void init(vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags properties);
    void copy(vk::CommandPool &command_pool, Buffer &dest);
    Buffer get_staging();
    void store(const void *src);
    void close();

private:
    Context &context;
};