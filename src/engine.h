#pragma once

#include "context.h"
#include "renderer.h"
#include "window.h"
#include "object.h"

#include <chrono>
class Renderer;

class Clock {
public:
	void update();
	float time = 0;
	float delta_time = 0;

	Clock() : last_frame(std::chrono::high_resolution_clock::now()) {};
private:
	std::chrono::steady_clock::time_point last_frame;
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
	Engine() : renderer(context) {}
	Context& get_context();
	Object& create_object();
	MeshObject& create_meshobject();

	Clock clock;

private:
	
	Context context;
	
};

