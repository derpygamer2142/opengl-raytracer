main: main.cpp fragment.glsl vertex.glsl
	g++ -I./include main.cpp include/glad.c include/stb_image.cpp -lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -o main