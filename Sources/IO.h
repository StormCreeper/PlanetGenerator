#pragma once

#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class Mesh;

class IO {
   private:
	static size_t write_data(void* ptr, size_t size, size_t nmemb, void* userdata);

   public:
	static std::string file2String(const std::string& filename);
	static void savePPM(const std::string& filename, int width, int height, const std::vector<glm::vec3>& pixels);
	static bool fetchTilePNG(int z, int x, int y, const std::string& outFilename);
};