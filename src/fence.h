#pragma once

#include "context.h"
#include "log.h"

class Fence {
public:
    vk::Fence fence = nullptr;
    Fence(Context &ctx, bool create_signalled) : context(ctx)
    {
        vk::FenceCreateInfo create_info(create_signalled ? vk::FenceCreateFlagBits::eSignaled : (vk::FenceCreateFlagBits)0);
        fence = context.device.createFence(create_info);
    }
    Fence(const Fence &copy) = delete;
    Fence(Fence &&move) : context(move.context), fence(move.fence) {
        move.fence = nullptr;
    }
    ~Fence()
    {
        if (fence != vk::Fence(nullptr)) {
            vkDestroyFence(context.device, fence, nullptr);
        }
    }

private:
    Context &context;
};