#include "asset_manager.h"

#include <iostream>
#include <fstream>
#include <filesystem>

const std::string AssetManager::ASSETS_FILE_PATH = "assets/assets.json";

void AssetManager::load_assets()
{
	using namespace nlohmann;
	std::ifstream assets_file(ASSETS_FILE_PATH);
	json assets_json;
	assets_file >> assets_json;

	AssetsList assets_list = assets_json.get<AssetsList>();
	std::cout << "loading textures" << std::endl;
	for (auto &tex : assets_list.textures) {
		tex.texture = Texture::load_image(context, tex.path, tex.color_space);
		textures[tex.name] = std::move(tex);
	}
	std::cout << "loaded " << textures.size() << " texture assets" << std::endl;


	std::cout << "loading models" << std::endl;
	for (auto& model : assets_list.models) {
		std::filesystem::path path = model.path;
		if (path.extension() == ".gltf") {
			model.model = Model::load_gltf(context, model.path);
		}
		else if (path.extension() == ".fbx" || path.extension() == ".obj") {
			model.model = Model::load_fbx(context, model.path);
		}
		else {
			std::string err = "unknown extension in asset manager model " + model.name;
			throw std::runtime_error(err);
		}
		
		models[model.name] = std::move(model);
	}
	std::cout << "loaded " << models.size() << " model assets" << std::endl;
}

void AssetManager::register_texture(const std::string &name, const std::string& path, ColorSpace color_space)
{
	auto asset = TextureAsset{
		name,
		path,
		color_space,
		Texture::load_image(context, path, color_space)
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
		res->init();
		res->upload(true);
	}

	return res;
}

Model* AssetManager::get_model(const std::string& name)
{
	if (models.find(name) == models.end()) {
		throw std::runtime_error("tried to get unregistered model " + name);
	}

	auto res = models[name].model.get();
	return res;
}

void AssetManager::close()
{
	textures.clear();
}
