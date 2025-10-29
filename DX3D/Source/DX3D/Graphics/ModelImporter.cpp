#include <DX3D/Graphics/ModelImporter.h>
#include <DX3D/Graphics/MaterialLoader.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Math/Vec2.h>
#include <DX3D/Math/Vec3.h>
#include <array>
#include <cfloat>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace dx3d
{
	namespace
	{

		struct VertexKey
		{
			Vec3 position{};
			Vec3 normal{};
			Vec2 texcoord{};

			bool operator==(const VertexKey& other) const noexcept
			{
				return position == other.position &&
					normal == other.normal &&
					texcoord == other.texcoord;
			}
		};

		struct VertexKeyHasher
		{
			size_t operator()(const VertexKey& key) const noexcept
			{
				auto h1 = std::hash<float>{}(key.position.x) ^ (std::hash<float>{}(key.position.y) << 1) ^ (std::hash<float>{}(key.position.z) << 2);
				auto h2 = std::hash<float>{}(key.normal.x) ^ (std::hash<float>{}(key.normal.y) << 1) ^ (std::hash<float>{}(key.normal.z) << 2);
				auto h3 = std::hash<float>{}(key.texcoord.x) ^ (std::hash<float>{}(key.texcoord.y) << 1);
				return h1 ^ (h2 << 1) ^ (h3 << 2);
			}
		};

		inline void parseVertexLine(std::istringstream& iss, std::vector<Vec3>& positions)
		{
			Vec3 pos{};
			iss >> pos.x >> pos.y >> pos.z;
			positions.push_back(pos);
		}

		inline void parseNormalLine(std::istringstream& iss, std::vector<Vec3>& normals)
		{
			Vec3 n{};
			iss >> n.x >> n.y >> n.z;
			normals.push_back(n);
		}

		inline void parseTexcoordLine(std::istringstream& iss, std::vector<Vec2>& texcoords)
		{
			Vec2 uv{};
			iss >> uv.x >> uv.y;
			uv.x = 1.0f - uv.x;
			uv.y = 1.0f - uv.y; // Flip Y for DirectX
			texcoords.push_back(uv);
		}

		inline void parseFaceToken(const std::string_view token, unsigned& vi, unsigned& ti, unsigned& ni)
		{
			vi = ti = ni = 0;

			size_t pos1 = token.find('/');
			size_t pos2 = token.find('/', pos1 + 1);

			if (pos1 == std::string::npos)
			{
				vi = std::stoi(std::string(token));
				return;
			}

			vi = std::stoi(std::string(token.substr(0, pos1)));

			if (pos2 != std::string::npos && pos2 > pos1 + 1)
				ti = std::stoi(std::string(token.substr(pos1 + 1, pos2 - pos1 - 1)));

			if (pos2 != std::string::npos && pos2 + 1 < token.size())
				ni = std::stoi(std::string(token.substr(pos2 + 1)));
		}

		inline void handleMaterialLib(
			std::istringstream& iss,
			const std::filesystem::path& fullPath,
			ModelData& model,
			const std::shared_ptr<GraphicsDevice>& graphicsDevice)
		{
			std::string mtlFile;
			iss >> mtlFile;

			model.materials = MaterialLoader::loadMTL(fullPath.string(), mtlFile);

			for (auto& mat : model.materials)
			{
				if (!mat.diffuseTexturePath.empty())
				{
					auto texPath = fullPath.parent_path() / mat.diffuseTexturePath;
					mat.diffuseTexture = graphicsDevice->createTexture2D(texPath.string());
				}
			}
		}

		inline void handleUseMaterial(std::istringstream& iss, ModelData& model, Material& currentMaterial)
		{
			std::string matName;
			iss >> matName;

			if (model.materialGroups.empty())
			{
				model.materialGroups.push_back({
					"default",
					0u,
					static_cast<unsigned>(model.indices.size()),
					-1
					});
			}
			else
			{
				model.materialGroups.back().indexCount =
					static_cast<unsigned>(model.indices.size()) -
					model.materialGroups.back().startIndex;
			}

			auto it = std::find_if(model.materials.begin(), model.materials.end(),
				[&](const Material& m) { return m.name == matName; });

			if (it != model.materials.end())
			{
				int matIndex = static_cast<int>(std::distance(model.materials.begin(), it));
				model.materialGroups.push_back({
					matName,
					static_cast<unsigned>(model.indices.size()),
					0,
					matIndex
					});

				currentMaterial = *it;
			}
		}

		inline void handleFaceLine(
			std::istringstream& iss,
			const std::vector<Vec3>& positions,
			const std::vector<Vec3>& normals,
			const std::vector<Vec2>& texcoords,
			std::unordered_map<VertexKey, uint32_t, VertexKeyHasher>& vertexLookup,
			const Material& currentMaterial,
			ModelData& model,
			Vec3& bbMin, Vec3& bbMax)
		{
			std::vector<std::string> tokens;
			std::string token;
			while (iss >> token) tokens.push_back(token);
			if (tokens.size() < 3) return;

			// triangulate (fan)
			for (size_t i = 1; i + 1 < tokens.size(); ++i)
			{
				std::array<std::string, 3> tri = { tokens[0], tokens[i], tokens[i + 1] };

				for (int j = 0; j < 3; ++j)
				{
					unsigned vi = 0, ti = 0, ni = 0;
					parseFaceToken(tri[j], vi, ti, ni);

					VertexKey key;
					key.position = (vi > 0 && vi <= positions.size()) ? positions[vi - 1] : Vec3{};
					key.texcoord = (ti > 0 && ti <= texcoords.size()) ? texcoords[ti - 1] : Vec2{};
					key.normal = (ni > 0 && ni <= normals.size()) ? normals[ni - 1] : Vec3{ 0,0,0 };

					if (ti > 0) model.hasTexcoords = true;
					if (ni > 0) model.hasNormals = true;

					auto it = vertexLookup.find(key);
					if (it != vertexLookup.end())
					{
						model.indices.push_back(it->second);
					}
					else
					{
						uint32_t newIndex = static_cast<uint32_t>(model.vertices.size());
						vertexLookup[key] = newIndex;

						Vec4 color(
							currentMaterial.diffuseColor.x,
							currentMaterial.diffuseColor.y,
							currentMaterial.diffuseColor.z,
							1.0f
						);

						model.vertices.emplace_back(key.position, key.normal, key.texcoord, color);
						model.indices.push_back(newIndex);

						// Update AABB
						bbMin.x = std::min(bbMin.x, key.position.x);
						bbMin.y = std::min(bbMin.y, key.position.y);
						bbMin.z = std::min(bbMin.z, key.position.z);
						bbMax.x = std::max(bbMax.x, key.position.x);
						bbMax.y = std::max(bbMax.y, key.position.y);
						bbMax.z = std::max(bbMax.z, key.position.z);
					}
				}
			}
		}

	}

	ModelData ModelImporter::loadOBJ(const std::string& relativePath)
	{
		ModelData model;
		model.sourcePath = relativePath;

		std::filesystem::path fullPath = std::filesystem::path("DX3D/Assets") / relativePath;
		std::ifstream file(fullPath);
		if (!file.is_open())
		{
			DX3D_LOG_ERROR("Failed to open OBJ file: {}", fullPath.string());
			return model;
		}

		DX3D_LOG_INFO("Loading model: {}", fullPath.string());

		std::vector<Vec3> positions;
		std::vector<Vec3> normals;
		std::vector<Vec2> texcoords;
		std::unordered_map<VertexKey, uint32_t, VertexKeyHasher> vertexLookup;
		Material currentMaterial;

		Vec3 bbMin{ FLT_MAX,  FLT_MAX,  FLT_MAX };
		Vec3 bbMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

		std::string line;
		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '#') continue;

			std::istringstream iss(line);
			std::string prefix; iss >> prefix;

			if (prefix == "v")      parseVertexLine(iss, positions);
			else if (prefix == "vn")     parseNormalLine(iss, normals);
			else if (prefix == "vt")     parseTexcoordLine(iss, texcoords);
			else if (prefix == "mtllib") handleMaterialLib(iss, fullPath, model, m_graphicsDevice);
			else if (prefix == "usemtl") handleUseMaterial(iss, model, currentMaterial);
			else if (prefix == "f")
				handleFaceLine(iss, positions, normals, texcoords,
					vertexLookup, currentMaterial, model, bbMin, bbMax);
		}
		file.close();

		// Finalize last material group
		if (!model.materialGroups.empty())
		{
			model.materialGroups.back().indexCount =
				static_cast<unsigned>(model.indices.size()) - model.materialGroups.back().startIndex;
		}

		// Store AABB (if we got any vertices)
		if (!model.vertices.empty())
			model.boundingBox = { bbMin, bbMax };

		// ---- Auto-generate normals if missing ----
		if (!model.hasNormals && !model.indices.empty())
		{
			DX3D_LOG_WARNING("Model has no normals, generating smooth normals...");
			// zero-out normals first
			for (auto& v : model.vertices) v.normal = Vec3{ 0,0,0 };

			for (size_t i = 0; i < model.indices.size(); i += 3)
			{
				uint32_t i0 = model.indices[i + 0];
				uint32_t i1 = model.indices[i + 1];
				uint32_t i2 = model.indices[i + 2];

				const Vec3& p0 = model.vertices[i0].position;
				const Vec3& p1 = model.vertices[i1].position;
				const Vec3& p2 = model.vertices[i2].position;

				Vec3 n = cross(p1 - p0, p2 - p0).normalize();
				model.vertices[i0].normal += n;
				model.vertices[i1].normal += n;
				model.vertices[i2].normal += n;
			}
			for (auto& v : model.vertices)
				v.normal = v.normal.normalize();

			model.hasNormals = true;
		}

		DX3D_LOG_INFO("Loaded {} vertices, {} indices ({} triangles) from {} | Normals:{} UVs:{}",
			model.vertices.size(),
			model.indices.size(),
			model.indices.size() / 3,
			fullPath.string(),
			model.hasNormals ? "Y" : "N",
			model.hasTexcoords ? "Y" : "N");

		return model;
	}
}
