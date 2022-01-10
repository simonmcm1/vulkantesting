#pragma once
#include "context.h"
#include "texture.h"
#include <unordered_map>

struct TextureAsset {
	std::string name;
	std::string path;
	std::unique_ptr<Texture> texture;
};

class AssetManager {
public:
	AssetManager(Context& ctx) : context(ctx) {}
	void register_texture(const std::string &name, const std::string& path);
	Texture* get_texture(const std::string& name);
	void close();
private:
	Context& context;
	std::unordered_map<std::string, TextureAsset> textures;
};