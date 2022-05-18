#pragma once

#include "context.h"
#include "asset_manager.h"
#include "renderer.h"
#include "window.h"
#include "object.h"
#include "texture.h"

#include <chrono>
class Renderer;

class Clock {
    using clock = std::chrono::steady_clock;
public:
	void update();
	float time = 0;
	float delta_time = 0;

	Clock() : last_frame(clock::now()) {};
private:
	clock::time_point last_frame;
};

class Engine
{
public:
	void init(Window &window);
	void execute_frame();
	void close();

	std::vector<std::unique_ptr<Object>> objects;
	std::unique_ptr<Camera> camera;
	Renderer renderer;
	AssetManager asset_manager;
	Engine() :
		asset_manager(context), 
		renderer(context, asset_manager) {}
	Context& get_context();
	Object& create_object();
	MeshObject& create_meshobject();
	SceneObject& create_sceneobject();
	Texture& create_texture(const std::string &filepath);

	Clock clock;

private:
	
	Context context;
	
};

