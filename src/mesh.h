#pragma once
#include "geometry.h"
#include "buffer.h"

class Renderer;

class MeshRenderer {
public:
	MeshRenderer(Context& ctx) : context(ctx), vertex_buffer(ctx), index_buffer(ctx), mesh(nullptr) {};
	Buffer vertex_buffer;
	Buffer index_buffer;
	const Mesh *mesh;
	MeshRenderer(MeshRenderer& other) = delete;

	void load(const Mesh &mesh);
	void init(Renderer &renderer);
	void close();

	void command_buffer(vk::CommandBuffer& cb);

private:
	Context& context;
};