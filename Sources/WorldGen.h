#pragma once

#include <glm/glm.hpp>

#include <FastNoise/FastNoise.h>

class WorldGen {
   private:
	inline void addFace(glm::vec3 xdir, glm::vec3 ydir, int subdivisions,
						std::vector<glm::vec3>& vertices,
						std::vector<glm::uvec3>& indices) {
		glm::vec3 zdir = glm::cross(xdir, ydir);

		unsigned int offset = vertices.size();
		for (int i = 0; i < subdivisions; i++) {
			for (int j = 0; j < subdivisions; j++) {
				// 0 to subdivisions - 1 => -1 to 1
				float x = 2.0f * i / (float)(subdivisions - 1) - 1.0f;
				float y = 2.0f * j / (float)(subdivisions - 1) - 1.0f;
				glm::vec3 vertex = x * xdir + y * ydir + zdir;
				vertices.push_back(vertex);
			}
		}

		for (int i = 0; i < subdivisions - 1; i++) {
			for (int j = 0; j < subdivisions - 1; j++) {
				// 0 1
				// 2 3
				int v0 = i * subdivisions + j;
				int v1 = i * subdivisions + j + 1;
				int v2 = (i + 1) * subdivisions + j;
				int v3 = (i + 1) * subdivisions + j + 1;
				indices.push_back(glm::uvec3(v0, v2, v1) + offset);
				indices.push_back(glm::uvec3(v1, v2, v3) + offset);
			}
		}
	}

	inline float getHeight(const glm::vec3& normPos,
						   FastNoise::SmartNode<FastNoise::FractalFBm>& fn) {
		float height = fn->GenSingle3D(normPos.x, normPos.y, normPos.z, 0);

		height = 0.5f * height + 0.5f;
		float ocean = 1.0f - height;
		height = glm::pow(height, 4);
		ocean = 1.0f - glm::pow(ocean, 3);

		// Ocean == 1 -> height = 0, ocean = 0 -> height = height
		height = glm::smoothstep(0.0f, 1.0f, ocean) * height;

		return 1 + height * 0.5f;
	}

   public:
	// Generate a sphere mesh with a given number of subdivisions
	inline void generateSphereMesh(int subdivisions,
								   std::vector<glm::vec3>& vertices,
								   std::vector<glm::uvec3>& indices) {
		glm::vec3 xdir = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 ydir = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 zdir = glm::vec3(0.0f, 0.0f, 1.0f);

		addFace(xdir, ydir, subdivisions, vertices, indices);
		addFace(-xdir, ydir, subdivisions, vertices, indices);
		addFace(-ydir, zdir, subdivisions, vertices, indices);
		addFace(ydir, zdir, subdivisions, vertices, indices);
		addFace(-xdir, zdir, subdivisions, vertices, indices);
		addFace(xdir, zdir, subdivisions, vertices, indices);

		auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
		auto fnFractal = FastNoise::New<FastNoise::FractalFBm>();
		fnFractal->SetSource(fnSimplex);
		fnFractal->SetOctaveCount(10);

#pragma omp parallel for
		for (glm::vec3& vertex : vertices) {
			vertex = glm::normalize(vertex);
			vertex *= getHeight(vertex, fnFractal);
		}
	}
};