#include "modelloader.h"

vector<string> strsplit(string input, char delimeter) {
    stringstream stream(input);

    string temp;

    vector<string> val;
    
    while (getline(stream, temp, delimeter)) {
        val.push_back(temp);
        //std::cout << "Split item " << temp << std::endl;
    }

    return val;
}

void Model::translate(float t0, float t1, float t2) {
    this->_translation0 = t0;
    this->_translation1 = t1;
    this->_translation2 = t2;
}

void Model::scale(float scale) {
    this->_scale = scale;
}

// Todo: Change this
void Model::transform() {
    for (int i = 0; i < this->_vertices.size(); i += 3) {
        float v0 = this->_vertices[i];
        float v1 = this->_vertices[i+1];
        float v2 = this->_vertices[i+2];

        v0 *= this->_scale;
        v1 *= this->_scale;
        v2 *= this->_scale;

        // Rotation goes here

        v0 += this->_translation0;
        v1 += this->_translation1;
        v2 += this->_translation2;

        this->final_vertices.push_back(v0);
        this->final_vertices.push_back(v1);
        this->final_vertices.push_back(v2);
    }
}

unsigned int Model::genVertexBuffer(float** output) {
    unsigned int size = (this->final_vertices.size()/3)*4;
    float* temp = new float[size];


    // Fill vertices
    int vIndex = 0;
    for (int i = 0; i < size; i += 4) {
        temp[i] = this->final_vertices[vIndex];
        temp[i+1] = this->final_vertices[vIndex+1];
        temp[i+2] = this->final_vertices[vIndex+2];

        vIndex += 3;
    }

    // Material corresponds to the face, not the vertex
    // We need to use the index buffer to figure out where to put the materials
    int mIndex = 0;
    for (int i = 0; i < this->triangles.size(); i++) {
        int vIndex = this->triangles[i]*4;
        temp[vIndex+3] = this->triMaterials[i/3];
    }


    std::cout << "Material list length: " << this->triMaterials.size() << ", index: " << mIndex << std::endl;

    *output = temp;
    return size;
}

unsigned int Model::genMaterialBuffer(float** output) {
    // I used this calculator for std 430
    // https://jackjamison.net/interactive/glsl-layout-calculator/
    unsigned int size = this->materials.size() * 12; // There's a bunch of properties but just trust me bro
    float* temp = new float[size];
    int j = 0;
    for (int i = 0; i < size; i += 12) {
        Material* mat = this->materials[i/12].get();
        for (int j = 0; j < 3; j++) temp[i+j] = mat->ambientColor[j]; // Float #3 is padding
        for (int j = 4; j < 7; j++) temp[i+j] = mat->diffuseColor[j-4];
        temp[i+7] = mat->ior;
        temp[i+8] = mat->roughness;
        temp[i+9] = mat->metallic;
        temp[i+10] = mat->sheen;
        temp[i+11] = mat->emission;
    }
    
    *output = temp;
    return size;
}

void Model::loadMTL(filesystem::path mtl) {
    ifstream file;
    file.open(mtl);
    if (!file.is_open()) {
        std::cout << "Failed to load MTL file." << std::endl;
        return;
    }
    
    std::unique_ptr<Material> mat = nullptr;

    string line;
    while (getline(file, line)) {
        std::cout << "Line: "<< line << std::endl;
        vector<string> tokens = strsplit(line, ' ');
        if (tokens.size() < 1) continue;

        if (tokens[0] == "newmtl") {
            if (mat) {
                this->materials.push_back(std::move(mat));
            }
            mat = std::make_unique<Material>();
            mat->ambientColor = std::make_unique<float[]>(3);
            mat->diffuseColor = std::make_unique<float[]>(3);
            mat->emission = 0.0f;
            mat->ior = 1.5f;
            mat->metallic = 0.0f;
            mat->roughness = 0.5f;
            mat->sheen = 0.0f;
            mat->name = tokens[1];
        }
        else if (tokens[0] == "Ka") {
            if (tokens.size() != 4) {
                std::cout << "Invalid number of tokens following " << tokens[0] << std::endl;
            }

            for (int i = 0; i < 3; i++) {
                mat->ambientColor[i] = stof(tokens[i+1]);
            }
        }
        else if (tokens[0] == "Kd") {
            if (tokens.size() != 4) {
                std::cout << "Invalid number of tokens following " << tokens[0] << std::endl;
            }

            for (int i = 0; i < 3; i++) {
                mat->diffuseColor[i] = stof(tokens[i+1]);
            }
        }
        else if (tokens[0] == "Ke") {
            if (tokens.size() != 4) {
                std::cout << "Invalid number of tokens following " << tokens[0] << std::endl;
            }

            float sum = 0.0f; // Only doing strength right now

            for (int i = 1; i < 4; i++) {
                sum += stof(tokens[i]);
            }

            mat->emission = sum/3.0f;
        }
        else if (tokens[0] == "Ni") {
            if (tokens.size() != 2) {
                std::cout << "Invalid number of tokens following " << tokens[0] << std::endl;
            }

            mat->ior = stof(tokens[1]);
        }
        else if (tokens[0] == "Pm") {
            if (tokens.size() != 2) {
                std::cout << "Invalid number of tokens following " << tokens[0] << std::endl;
            }

            mat->metallic = stof(tokens[1]);
        }
        else if (tokens[0] == "Pr") {
            if (tokens.size() != 2) {
                std::cout << "Invalid number of tokens following " << tokens[0] << std::endl;
            }

            mat->roughness = stof(tokens[1]);
        }
        else if (tokens[0] == "Ps") {
            if (tokens.size() != 2) {
                std::cout << "Invalid number of tokens following " << tokens[0] << std::endl;
            }

            mat->sheen = stof(tokens[1]);
        }
        // todo: textures
    }

    if (mat) this->materials.push_back(std::move(mat));
}

void Model::loadMTL(string mtl, filesystem::path path) {
    this->loadMTL(path.parent_path() / mtl);
}

Model loadOBJ(filesystem::path path) {
    ifstream file;
    file.open(path);
    if (!file.is_open()) {
        std::cout << "Failed to load model." << std::endl;
        
        return Model();
    }

    string line;

    Model model;
    int currentMaterial = 0;

    while (getline(file, line)) {
        //std::cout << "Read line " << line << std::endl;
        vector<string> tokens = strsplit(line, ' ');
        if (tokens.size() < 1) continue;

        if (tokens[0] == "v") {
            if (tokens.size() < 4) {
                std::cout << "Problem when loading model, type V - Expected 4 tokens, received " << tokens.size() << ". Line:\n" << line << std::endl;
                continue;
            }

            for (int i = 1; i < 4; i++) {
                model._vertices.push_back(stof(tokens[i])); // Convert tokens 1 through 3 to floats and add them to the vertex array
                //std::cout << "Append vertex" << std::endl;
            }
        }

        else if (tokens[0] == "f") {
            if (tokens.size() != 4) {
                std::cout << "Problem when loading model, type F - Expected 4 tokens, received " << tokens.size() << ". Line:\n" << line << "\nOnly triangulated models are supported." << std::endl;
                continue;
            }

            for (int i = 1; i < 4; i++) {
                vector<string> face = strsplit(tokens[i], '/'); // This also has the texture coordinate, but that hasn't been implemented yet

                model.triangles.push_back(static_cast<unsigned int>(stoul(face[0])) - 1u);
                if (model.materials[currentMaterial]->emission > 0.0) {
                    model.lights.push_back(static_cast<unsigned int>(stoul(face[0])) - 1u);
                }
                //std::cout << "Append triangle" << std::endl;
            }
            model.triMaterials.push_back(static_cast<unsigned int>(currentMaterial));
        }

        else if (tokens[0] == "mtllib") {
            // Note: this won't work if the mtl file has a space in the name
            // One specification allows multiple file names
            std::cout << "Loading mtl" << std::endl;
            model.loadMTL(tokens[1], path);
        }
        else if (tokens[0] == "usemtl"){
            currentMaterial = 0;
            for (int i = 0; i < model.materials.size(); i++) {
                std::cout << "Checking " << model.materials[i]->name << ", " << tokens[1] << std::endl;
                if (model.materials[i]->name == tokens[1]) {
                    currentMaterial = i;
                    break;
                }
            }
        }
    }


    return model;
}

Model loadOBJ(const char* path) {
    return loadOBJ(filesystem::path(path));
}