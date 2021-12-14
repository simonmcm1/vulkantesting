#pragma once

#include "context.h"

class Semaphore {
public:
    vk::Semaphore semaphore = nullptr;
    Semaphore(Context &ctx)  : context(ctx) {
        vk::SemaphoreCreateInfo create_info{};
        semaphore = context.device.createSemaphore(create_info);
    }
    Semaphore(const Semaphore &copy) = delete;
    Semaphore(Semaphore &&move) : context(move.context), semaphore(move.semaphore) {
        move.semaphore = nullptr;
    }
    ~Semaphore()
    {
        if (semaphore != vk::Semaphore(nullptr)) {
            vkDestroySemaphore(context.device, semaphore, nullptr);
        }
    }

private:
    Context &context;
};