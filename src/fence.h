#pragma once

#include "context.h"

class Fence {
public:
    VkFence fence;
    Fence(Context &ctx, bool create_signalled)  : context(ctx) {
        VkFenceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.flags = create_signalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

        if(vkCreateFence(context.device, &create_info, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence");
        }
    }
    Fence(const Fence &copy) = delete;
    Fence(Fence &&move) : context(move.context), fence(move.fence) {
        move.fence = nullptr;
    }
    ~Fence()
    {
        if (fence != nullptr) {
            vkDestroyFence(context.device, fence, nullptr);
        }
    }

private:
    Context &context;
};