#include "IO.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <ios>
#include <sstream>

#include <algorithm>

#include <curl/curl.h>

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

size_t IO::write_data(void* ptr, size_t size, size_t nmemb, void* userdata) {
	std::cout << "write_data called\n";
	auto* buffer = static_cast<std::vector<unsigned char>*>(userdata);
	size_t total = size * nmemb;
	unsigned char* data = static_cast<unsigned char*>(ptr);
	buffer->insert(buffer->end(), data, data + total);
	return total;
}

bool IO::fetchTilePNG(int z, int x, int y, const std::string& outFilename) {
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
	std::vector<unsigned char> pngData;

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

	// 6) Save to file
	std::ofstream fout(outFilename, std::ios::binary);
	if (!fout) {
		std::cerr << "Failed to open output file: " << outFilename << "\n";
		return false;
	}
	fout.write(reinterpret_cast<const char*>(pngData.data()), pngData.size());
	fout.close();

	std::cout << "Saved tile to " << outFilename << " ("
			  << pngData.size() << " bytes)\n";
	return true;
}