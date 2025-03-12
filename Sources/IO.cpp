#include "IO.h" 

#include <iostream>
#include <fstream>
#include <exception>
#include <ios>
#include <sstream>

#include <algorithm>

std::string IO::file2String(const std::string& filename) {
    std::ifstream input(filename.c_str());
    if (!input)
        throw std::ios_base::failure("[Shader Program][file2String] Error: cannot open " + filename);
    std::stringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void IO::savePPM(const std::string& filename, int width, int height, const std::vector<glm::vec3>& pixels) {
    std::ofstream out(filename.c_str());
    if (!out) {
        std::cerr << "Cannot open file " << filename.c_str() << std::endl;
        std::exit(1);
    }
    out << "P3" << std::endl
        << width << " " << height << std::endl
        << "255" << std::endl;
    for (size_t y = 0; y < height; y++)
        for (size_t x = 0; x < width; x++) {
            out << std::min(255u, static_cast<unsigned int>(255.f * pixels[y * width + x][0])) << " "
                << std::min(255u, static_cast<unsigned int>(255.f * pixels[y * width + x][1])) << " "
                << std::min(255u, static_cast<unsigned int>(255.f * pixels[y * width + x][2])) << " ";
        }
    out << std::endl;
    out.close();
}