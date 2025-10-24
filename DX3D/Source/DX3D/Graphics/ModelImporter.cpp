#include <DX3D/Graphics/ModelImporter.h>
#include <DX3D/Graphics/MaterialLoader.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Math/Vec2.h>
#include <DX3D/Math/Vec3.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

namespace dx3d
{
    struct VertexKey
    {
        Vec3 position;
        Vec3 normal;
        Vec2 texcoord;

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

    ModelData ModelImporter::loadOBJ(const std::string& relativePath)
    {
        ModelData model;
        Material currentMaterial;

        model.sourcePath = relativePath;

        std::filesystem::path fullPath = std::filesystem::path("DX3D\\Assets") / relativePath;
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

        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v")
            {
                Vec3 pos;
                iss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            }
            else if (prefix == "vn")
            {
                Vec3 n;
                iss >> n.x >> n.y >> n.z;
                normals.push_back(n);
            }
            else if (prefix == "vt")
            {
                Vec2 uv;
                iss >> uv.x >> uv.y;
                uv.y = 1.0f - uv.y; // flip Y for DirectX
                texcoords.push_back(uv);
            }
            else if (prefix == "mtllib")
            {
                std::string mtlFile;
                iss >> mtlFile;
                model.materials = MaterialLoader::loadMTL(fullPath.string(), mtlFile);
            }
            else if (prefix == "usemtl")
            {
                std::string matName;
                iss >> matName;

                auto it = std::find_if(model.materials.begin(), model.materials.end(),
                    [&](const Material& m) { return m.name == matName; });

                if (it != model.materials.end())
                    currentMaterial = *it;
            }
            else if (prefix == "f")
            {
                std::vector<std::string> tokens;
                std::string token;
                while (iss >> token)
                    tokens.push_back(token);

                if (tokens.size() < 3)
                    continue;

                // triangulate fan method
                for (size_t i = 1; i + 1 < tokens.size(); ++i)
                {
                    std::string tri[3] = { tokens[0], tokens[i], tokens[i + 1] };

                    for (int j = 0; j < 3; ++j)
                    {
                        unsigned int vi = 0, ti = 0, ni = 0;
                        if (sscanf_s(tri[j].c_str(), "%u/%u/%u", &vi, &ti, &ni) == 0)
                            sscanf_s(tri[j].c_str(), "%u//%u", &vi, &ni);

                        VertexKey key;
                        key.position = (vi > 0 && vi <= positions.size()) ? positions[vi - 1] : Vec3{};
                        key.texcoord = (ti > 0 && ti <= texcoords.size()) ? texcoords[ti - 1] : Vec2{};
                        key.normal = (ni > 0 && ni <= normals.size()) ? normals[ni - 1] : Vec3{ 0, 1, 0 };

                        auto it = vertexLookup.find(key);
                        if (it != vertexLookup.end())
                        {
                            // already exists
                            model.indices.push_back(it->second);
                        }
                        else
                        {
                            uint32_t newIndex = static_cast<uint32_t>(model.vertices.size());
                            vertexLookup[key] = newIndex;

                            Vec4 color(currentMaterial.diffuseColor.x,
                                currentMaterial.diffuseColor.y,
                                currentMaterial.diffuseColor.z,
                                1.0f);

                            model.vertices.emplace_back(key.position, key.normal, key.texcoord, color);
                            model.indices.push_back(newIndex);
                        }
                    }
                }
            }
        }

        file.close();

        DX3D_LOG_INFO("Loaded {} unique vertices, {} indices, {} triangles from {}",
            model.vertices.size(), model.indices.size(), model.indices.size() / 3, fullPath.string());

        return model;
    }
}
