all: build/shaders build/assets build/main build/main.exe

clean:
	rm -rf build

build/assets:
	mkdir -p build
	rm -rf build/assets
	cp -r src/assets build

build/shaders: src/shaders/fragment.glsl src/shaders/vertex.glsl src/shaders/textureoutput.glsl
	mkdir -p build
	rm -rf build/shaders
	cp -r src/shaders build

# Linux
build/main: src/main.cpp src/shader.cpp src/shader.h src/fileutil.h src/fileutil.cpp
	g++ -I./include \
	src/main.cpp src/shader.cpp src/fileutil.cpp \
	include/glad.c include/stb_image.cpp \
	-lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi -ldl \
	-o build/main


# Windows
build/main.exe: src/main.cpp src/shader.cpp src/shader.h src/fileutil.h src/fileutil.cpp
	x86_64-w64-mingw32-g++ -D GLFW_DLL \
		-I./include \
		src/main.cpp src/fileutil.cpp src/shader.cpp \
		-static-libgcc -static-libstdc++ \
		-static \
		include/stb_image.cpp \
		include/libglfw3dll.a \
		include/glad.c \
		-lglfw3 -lopengl32 -lgdi32 \
		-o build/main.exe

	cp include/glfw3.dll build/glfw3.dll