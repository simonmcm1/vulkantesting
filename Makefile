CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: src/*.cpp
	g++ $(CFLAGS) -o VulkanTest src/main.cpp src/window.cpp src/context.cpp src/renderer.cpp src/swapchain.cpp src/pipeline.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest

shaders:
	./compile_shaders.sh