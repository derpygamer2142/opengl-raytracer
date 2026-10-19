#ifndef MODEL_LOADER
#define MODEL_LOADER

#include <vector>
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <memory>
using namespace std;

vector<string> strsplit(string input, char delimeter);

struct Material {
    std::unique_ptr<float[]> ambientColor; // 3 component vector
    std::unique_ptr<float[]> diffuseColor;
    float ior; // Ni in an mtl file
    float roughness;
    float metallic; // Todo
    float sheen;    // Maybe todo?
    float emission;
    string name;
    // Excluded: anisotropy, clear coat, normal map, specular color, transparency, illumination model
};

class Model {
    public:
        vector<unsigned int> triangles;
        vector<unsigned int> triMaterials; // Which material a given triangle uses.
        // Wastes memory, but random lookup will be good when bvh's are added.
        vector<unsigned int> lights; // Array of triangles(same as the triangle list) that emit light.

        vector<std::unique_ptr<Material>> materials;

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

        // Because alignment
        unsigned int genVertexBuffer(float** output);
        unsigned int genMaterialBuffer(float** output);

        // Todo: Texture nonsense
        void loadMTL(filesystem::path mtl);
        void loadMTL(string mtl, filesystem::path path);
};


Model loadOBJ(filesystem::path path);
Model loadOBJ(const char* path);


#endif