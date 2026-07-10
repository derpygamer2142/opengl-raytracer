#ifndef MODEL_LOADER
#define MODEL_LOADER

#include <vector>
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
using namespace std;

vector<string> strsplit(string input, char delimeter);

class Model {
    public:
        vector<unsigned int> triangles;

        vector<float> _vertices;
        vector<float> final_vertices;

        float _translation0 = 0.0f;
        float _translation1 = 0.0f;
        float _translation2 = 0.0f;
        float _scale = 1.0f;
        // Todo: Rotation. Maybe use quaternions?

        // Translates the model by a given amount. Does not update until transform() is called
        void translate(float t0, float t1, float t2);

        // Scales the model around the origin by a given amount. Does not update until transform() is called
        void scale(float scale);

        // Calculates the final positions of every vertex
        void transform();

        // Todo: Texture nonsense
};


Model loadOBJ(filesystem::path path);
Model loadOBJ(const char* path);


#endif