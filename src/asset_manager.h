#pragma once
#include "context.h"
#include "texture.h"

#include <nlohmann/json.hpp>
#include <unordered_map>

struct TextureAsset {
	std::string name;
	std::string path;
	std::unique_ptr<Texture> texture;
};

struct AssetsList {
	std::vector<TextureAsset> textures;
};

class AssetManager {
public:
	AssetManager(Context& ctx) : context(ctx) {}
	void load_assets();
	void register_texture(const std::string &name, const std::string& path);
	Texture* get_texture(const std::string& name);
	void close();
private:
	static const std::string ASSETS_FILE_PATH;
	Context& context;
	std::unordered_map<std::string, TextureAsset> textures;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextureAsset, name, path)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AssetsList, textures)