#pragma once
#include "geometry.h"
#include "context.h"
#include "texture.h"
#include "transform.h"

struct SceneNode {
	std::vector<SceneNode*> children;
	std::string mesh;
	Transform transform;
	std::string name;
};

class Model {
public:
	std::unordered_map<std::string, Mesh> meshes;
	std::unordered_map<std::string, Texture> textures;
	std::unordered_map<std::string, SceneNode> nodes;
	SceneNode scene_root;
	

	static std::unique_ptr<Model> load_fbx(Context &context, const std::string& path);
	static std::unique_ptr<Model> load_gltf(Context& context, const std::string& path);
};
