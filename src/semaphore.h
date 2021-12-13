#pragma once

#include "context.h"

class Semaphore {
public:
    VkSemaphore semaphore;
    Semaphore(Context &ctx)  : context(ctx) {
        VkSemaphoreCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
        if(vkCreateSemaphore(context.device, &create_info, nullptr, &semaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore");
        }
    }
    Semaphore(const Semaphore &copy) = delete;
    Semaphore(Semaphore &&move) : context(move.context), semaphore(move.semaphore) {
        move.semaphore = nullptr;
    }
    ~Semaphore()
    {
        if (semaphore != nullptr) {
            vkDestroySemaphore(context.device, semaphore, nullptr);
        }
    }

private:
    Context &context;
};