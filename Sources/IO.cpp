#include "IO.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <ios>
#include <sstream>

#include <algorithm>

#include <curl/curl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/glad.h>

std::string IO::file2String(const std::string& filename) {
	std::ifstream input(filename.c_str());
	if (!input)
		throw std::ios_base::failure("[Shader Program][file2String] Error: cannot open " + filename);
	std::stringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();
}

// Save a PPM image to a file from a vector of pixels
// The pixels are expected to be in the range [0, 1] for each color channel
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

bool decodePNG(const std::vector<unsigned char>& pngData,
			   int& width, int& height,
			   std::vector<GLubyte>& outPixels) {
	// stbi_set_flip_vertically_on_load(true);

	int comp;
	unsigned char* img = stbi_load_from_memory(
		pngData.data(), int(pngData.size()),
		&width, &height, &comp, /*req_channels=*/3);
	if (!img) return false;

	size_t npix = size_t(width) * height;
	outPixels.reserve(npix);
	for (size_t i = 0; i < npix; ++i) {
		outPixels.push_back(img[i * 3 + 0]);
		outPixels.push_back(img[i * 3 + 1]);
		outPixels.push_back(img[i * 3 + 2]);
	}
	stbi_image_free(img);
	return true;
}

size_t IO::write_data(void* ptr, size_t size, size_t nmemb, void* userdata) {
	auto* buffer = static_cast<std::vector<unsigned char>*>(userdata);
	size_t total = size * nmemb;
	unsigned char* data = static_cast<unsigned char*>(ptr);
	buffer->insert(buffer->end(), data, data + total);
	return total;
}

bool IO::fetchTilePNG(int z, int x, int y, int& outWidth, int& outHeight, std::vector<GLubyte>& outPixels) {
	// 1) Build the URL
	char url[256];
	std::snprintf(url, sizeof(url),
				  "https://tile.openstreetmap.org/%d/%d/%d.png",
				  z, x, y);

	// 2) Initialize curl
	CURL* curl = curl_easy_init();
	if (!curl) {
		std::cerr << "Failed to init libcurl\n";
		return false;
	}

	// Buffer to hold the downloaded data
	std::vector<GLubyte> pngData;

	// 3) Set curl options
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "PlanetGen/1.0 (telo.philippe@gmail.com)");
	curl_easy_setopt(curl, CURLOPT_URL, url);
	// Follow HTTP redirects if any
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	// Write callback into our buffer
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pngData);
	// (Optional) set a reasonable timeout
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	// 4) Perform the request
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		std::cerr << "curl_easy_perform() failed: "
				  << curl_easy_strerror(res) << "\n";
		curl_easy_cleanup(curl);
		return false;
	}

	std::cout << "Downloaded tile from " << url << "\n";
	// 5) Clean up
	curl_easy_cleanup(curl);

	// Decode PNG data

	if (!decodePNG(pngData, outWidth, outHeight, outPixels)) {
		std::cerr << "Failed to decode PNG data\n";
		return false;
	}

	std::cout << "Decoded PNG data: " << outWidth << "x" << outHeight << ", tot: " << outPixels.size() << "\n";

	return true;
}

unsigned int IO::fetchTileToTexture(int z, int x, int y) {
	int width, height;
	std::vector<GLubyte> pixels;
	if (!fetchTilePNG(z, x, y, width, height, pixels)) {
		std::cerr << "Failed to fetch tile PNG\n";
		return 0;
	}

	std::cout << "Fetched tile PNG: " << width << "x" << height << ", tot: " << pixels.size() << "\n";

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	return textureID;
}
