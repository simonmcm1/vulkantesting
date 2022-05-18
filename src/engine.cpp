#include "engine.h"
#include <chrono>

void Clock::update()
{
	auto now = clock::now();
	time = std::chrono::duration_cast<std::chrono::duration<float>>(now.time_since_epoch()).count();
	delta_time = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame).count();
	last_frame = now;
}

void Engine::init(Window &window)
{
	context.init();
	window.create_surface();
	renderer.init();
	for (auto& object : objects) {
		object->init(*this);
	}
}

void Engine::execute_frame()
{
	clock.update();
	renderer.render(*camera, objects);
}

void Engine::close()
{
	renderer.wait_for_idle();
	for (auto& object : objects) {
		object->close();
	}
	asset_manager.close();
	renderer.close();
	context.close();
}

Context& Engine::get_context()
{
	return context;
}

Object& Engine::create_object()
{
	objects.push_back(std::make_unique<Object>(context));
	return *objects.back();
}

MeshObject& Engine::create_meshobject()
{
	auto mo = std::make_unique<MeshObject>(context);
	objects.push_back(std::move(mo));
	//TODO: wtf is this
	return dynamic_cast<MeshObject&>(*objects.back());
}

SceneObject& Engine::create_sceneobject()
{
	auto so = std::make_unique<SceneObject>(context);
	objects.push_back(std::move(so));
	//TODO: wtf is this
	return dynamic_cast<SceneObject&>(*objects.back());
}

