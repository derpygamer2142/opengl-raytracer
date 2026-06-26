#ifndef FILE_UTIL
#define FILE_UTIL
#include <iostream>
#include <filesystem>
#include <fstream>
using namespace std;


char* readFile(const char* name) {
	ifstream file;
	char* output;
	file.open(name, ios::in | ios::binary);

	if (file.is_open()) {
		int filesize = std::filesystem::file_size(name);

		output = new char[filesize+1]();
		file.read(output, filesize);
		output[filesize] = '\0';
		//std::cout << "Read file:\n" << output << "\n\nFAFFASAWGJHNadwwdawdfawdH\n\n" << std::endl;
		return output;
	}
	else std::cout << "Failed to open file '" << name << "'." << std::endl;
	return 0;
}


#endif