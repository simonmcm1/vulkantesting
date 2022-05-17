#pragma once
#include "geometry.h"
#include "context.h"

class Model {
public:
	std::unordered_map<std::string, Mesh> meshes;

	static std::unique_ptr<Model> load_fbx(Context &context, const std::string& path);
	static std::unique_ptr<Model> load_gltf(Context& context, const std::string& path);
};
