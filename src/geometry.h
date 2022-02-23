#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec3 tangent;

    static vk::VertexInputBindingDescription binding_description() {
        vk::VertexInputBindingDescription description(
            0, sizeof(Vertex), vk::VertexInputRate::eVertex);
        return description;
    }

    static std::array<vk::VertexInputAttributeDescription, 5> attribute_description() {
        std::array<vk::VertexInputAttributeDescription, 5> description{};
        //pos
        description[0].binding = 0;
        description[0].location = 0;
        description[0].format = vk::Format::eR32G32B32Sfloat;
        description[0].offset = offsetof(Vertex, pos);

        //color
        description[1].binding = 0;
        description[1].location = 1;
        description[1].format = vk::Format::eR32G32B32A32Sfloat;
        description[1].offset = offsetof(Vertex, color);

        //uv
        description[2].binding = 0;
        description[2].location = 2;
        description[2].format = vk::Format::eR32G32Sfloat;
        description[2].offset = offsetof(Vertex, uv);

        //normal
        description[3].binding = 0;
        description[3].location = 3;
        description[3].format = vk::Format::eR32G32B32Sfloat;
        description[3].offset = offsetof(Vertex, normal);

        //tangent
        description[4].binding = 0;
        description[4].location = 4;
        description[4].format = vk::Format::eR32G32B32Sfloat;
        description[4].offset = offsetof(Vertex, tangent);

        return description;
    }
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

const Mesh QUAD = {
    {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
     {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
     {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
     {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}},
    {0, 1, 2, 2, 3, 0}};
