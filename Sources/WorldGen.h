#pragma once

#include <glm/glm.hpp>

#include <FastNoise/FastNoise.h>

#include <math.h>
#define DEG2RAD(a) ((a) / (180 / M_PI))
#define RAD2DEG(a) ((a) * (180 / M_PI))
#define EARTH_RADIUS 6378137.0f

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

	inline float getHeight(const glm::vec3& normPos, FastNoise::SmartNode<FastNoise::FractalFBm>& fn) {
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

	// Inverse Web‑Mercator: from normalized v in [0,1] to latitude in radians
	inline float invMercatorLat(float v) {
		// y ∈ [−π, +π], with v=0 → y=+π, v=1 → y=−π
		float y = glm::pi<float>() * (1.0f - 2.0f * v);
		// φ = arctan(sinh(y))
		return atanf(sinhf(y));
	}

	void generateMercatorTileMesh(int z,								// zoom level (number of subdivisions)
								  std::vector<glm::vec2>& positions2D,	// UV coordinates
								  std::vector<glm::vec3>& positions3D,	// 3D positions on unit sphere
								  std::vector<glm::uvec3>& indices) {
		const uint32_t n = 1u << z;	 // number of squares per axis
		const uint32_t vertCount = (n + 1) * (n + 1);

		positions2D.reserve(vertCount);
		positions3D.reserve(vertCount);
		indices.reserve(n * n * 2);	 // 2 triangles per square

		// build the grid of (u,v)
		for (uint32_t y = 0; y <= n; ++y) {
			float v = float(y) / float(n);
			for (uint32_t x = 0; x <= n; ++x) {
				float u = float(x) / float(n);
				positions2D.emplace_back(u, v);

				// inverse‑Mercator-> lat/lon
				float lon = glm::two_pi<float>() * u - glm::pi<float>();  // [−π,π]
				float lat = invMercatorLat(v);							  // [−π/2,+π/2]

				// spherical to Cartesian on unit sphere
				float cosLat = cosf(lat);
				glm::vec3 p = {
					cosLat * cosf(lon),
					sinf(lat),
					cosLat * sinf(lon)};
				positions3D.push_back(p);
			}
		}

		auto idx = [&](uint32_t ix, uint32_t iy) {
			return iy * (n + 1) + ix;
		};

		for (uint32_t y = 0; y < n; ++y) {
			for (uint32_t x = 0; x < n; ++x) {
				uint32_t i0 = idx(x, y);
				uint32_t i1 = idx(x + 1, y);
				uint32_t i2 = idx(x, y + 1);
				uint32_t i3 = idx(x + 1, y + 1);

				indices.push_back({i0, i1, i2});
				indices.push_back({i1, i3, i2});
			}
		}
	}
};