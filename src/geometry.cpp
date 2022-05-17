#include "geometry.h"

//adapted from http://foundationsofgameenginedev.com/FGED2-sample.pdf
void RecalculateTangents(Mesh& mesh)
{

    std::vector<glm::vec3> bitangents(mesh.vertices.size(), glm::vec3(0, 0, 0));

    // Calculate tangent and bitangent for each triangle and add to all three vertices.
    for (uint32_t k = 0; k < mesh.indices.size() / 3; k++)
    {
        uint32_t i0 = mesh.indices[k];
        uint32_t i1 = mesh.indices[k + 1];
        uint32_t i2 = mesh.indices[k + 2];
        auto& v0 = mesh.vertices.at(i0);
        auto& v1 = mesh.vertices.at(i1);
        auto& v2 = mesh.vertices.at(i2);

        glm::vec3 e1 = v1.pos - v0.pos;
        glm::vec3 e2 = v2.pos - v0.pos;
        float x1 = v1.uv.x - v0.uv.x, x2 = v2.uv.x - v0.uv.x;
        float y1 = v1.uv.y - v0.uv.y, y2 = v2.uv.y - v0.uv.y;
        float r = 1.0F / (x1 * y2 - x2 * y1);
        glm::vec3 t = (e1 * y2 - e2 * y1) * r;
        glm::vec3 b = (e2 * x1 - e1 * x2) * r;

        v0.tangent += t;
        v1.tangent += t;
        v2.tangent += t;
        bitangents[i0] += b;
        bitangents[i1] += b;
        bitangents[i2] += b;
    }

    // Orthonormalize each tangent and calculate the handedness.
    for (uint32_t i = 0; i < mesh.vertices.size(); i++)
    {
        Vertex& v = mesh.vertices.at(i);
        const glm::vec3& t = v.tangent;
        const glm::vec3& b = bitangents[i];
        const glm::vec3& n = v.normal;

        v.tangent = glm::orthonormalize(t, n);

        // w component flips the bitangent if needed?
        //tangentArray[i].w = (Dot(Cross(t, b), n) > 0.0F) ? 1.0F : -1.0F;
    }
}