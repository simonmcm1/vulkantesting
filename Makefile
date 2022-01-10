CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
SOURCES=${wildcard src/*.cpp} 
INCLUDES = -Ithirdparty/stb

VulkanTest: src/*.cpp
	g++ $(CFLAGS) -o VulkanTest $(SOURCES) $(INCLUDES) $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest

shaders:
	./compile_shaders.sh