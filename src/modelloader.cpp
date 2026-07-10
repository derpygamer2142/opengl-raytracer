#include "modelloader.h"

vector<string> strsplit(string input, char delimeter) {
    stringstream stream(input);

    string temp;

    vector<string> val;
    
    while (getline(stream, temp, delimeter)) {
        val.push_back(temp);
        std::cout << "Split item " << temp << std::endl;
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

Model loadOBJ(filesystem::path path) {
    ifstream file;
    file.open(path);
    if (!file.is_open()) {
        std::cout << "Failed to load model." << std::endl;
        
        return Model();
    }

    string line;

    Model model;

    while (getline(file, line)) {
        std::cout << "Read line " << line << std::endl;
        vector<string> tokens = strsplit(line, ' ');
        if (tokens.size() < 1) continue;

        if (tokens[0] == "v") {
            if (tokens.size() < 4) {
                std::cout << "Problem when loading model, type V - Expected 4 tokens, received " << tokens.size() << ". Line:\n" << line << std::endl;
                continue;
            }

            for (int i = 1; i < 4; i++) {
                model._vertices.push_back(stof(tokens[i])); // Convert tokens 1 through 3 to floats and add them to the vertex array
                std::cout << "Append vertex" << std::endl;
            }
        }

        else if (tokens[0] == "f") {
            if (tokens.size() != 4) {
                std::cout << "Problem when loading model, type F - Expected 4 tokens, received " << tokens.size() << ". Line:\n" << line << "\nOnly triangulated models are supported." << std::endl;
                continue;
            }

            for (int i = 1; i < 4; i++) {
                vector<string> face = strsplit(tokens[i], '/'); // This also has the texture coordinate, but that hasn't been implemented yet

                model.triangles.push_back(static_cast<unsigned int>(stoul(face[0]) )-1u);
                std::cout << "Append triangle" << std::endl;
            }
        }
    }


    return model;
}

Model loadOBJ(const char* path) {
    return loadOBJ(filesystem::path(path));
}