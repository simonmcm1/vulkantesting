#include "asset_manager.h"

void AssetManager::register_texture(const std::string &name, const std::string& path)
{
	auto asset = TextureAsset{
		name,
		path,
		Texture::load_image(context, path)
	};
	textures[name] = std::move(asset);
}

Texture* AssetManager::get_texture(const std::string& name)
{
	if (textures.find(name) == textures.end()) {
		throw std::runtime_error("tried to get unregistered texture " + name);
	}

	auto res = textures[name].texture.get();
	if (!res->uploaded) {
		res->upload();
	}

	return res;
}

void AssetManager::close()
{
	textures.clear();
}
