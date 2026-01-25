#include <DX3D/Graphics/ModelImporter.h>
#include <DX3D/Graphics/MaterialLoader.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/AssetManager.h>
#include <DX3D/Core/Logger.h>

#include <DirectXMath.h>

#include <array>
#include <cfloat>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace dx3d
{
    using namespace DirectX;

    namespace
    {
        struct VertexKey
        {
            XMFLOAT3 position{};
            XMFLOAT3 normal{};
            XMFLOAT2 texcoord{};

            bool operator==(const VertexKey& other) const noexcept
            {
                return position.x == other.position.x &&
                    position.y == other.position.y &&
                    position.z == other.position.z &&
                    normal.x == other.normal.x &&
                    normal.y == other.normal.y &&
                    normal.z == other.normal.z &&
                    texcoord.x == other.texcoord.x &&
                    texcoord.y == other.texcoord.y;
            }
        };

        struct VertexKeyHasher
        {
            size_t operator()(const VertexKey& key) const noexcept
            {
                size_t h1 = std::hash<float>{}(key.position.x) ^
                    (std::hash<float>{}(key.position.y) << 1) ^
                    (std::hash<float>{}(key.position.z) << 2);

                size_t h2 = std::hash<float>{}(key.normal.x) ^
                    (std::hash<float>{}(key.normal.y) << 1) ^
                    (std::hash<float>{}(key.normal.z) << 2);

                size_t h3 = std::hash<float>{}(key.texcoord.x) ^
                    (std::hash<float>{}(key.texcoord.y) << 1);

                return h1 ^ (h2 << 1) ^ (h3 << 2);
            }
        };

        inline void parseVertexLine(std::istringstream& iss, std::vector<XMFLOAT3>& positions)
        {
            XMFLOAT3 pos{};
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }

        inline void parseNormalLine(std::istringstream& iss, std::vector<XMFLOAT3>& normals)
        {
            XMFLOAT3 n{};
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }

        inline void parseTexcoordLine(std::istringstream& iss, std::vector<XMFLOAT2>& texcoords)
        {
            XMFLOAT2 uv{};
            iss >> uv.x >> uv.y;
            uv.x = 1.0f - uv.x;
            uv.y = 1.0f - uv.y;
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
            const std::shared_ptr<GraphicsDevice>& graphicsDevice,
            AssetManager* assets)
        {
            std::string mtlFile;
            iss >> mtlFile;
            const std::filesystem::path mtlPath = fullPath.parent_path() / mtlFile;

            if (assets)
            {
                model.materials = assets->getMaterialsFromFile(fullPath.string(), mtlFile);
            }
            else
            {
                DX3D_LOG_INFO("Loading materials (no cache) from: {}", mtlPath.string());
                model.materials = MaterialLoader::loadMTL(fullPath.string(), mtlFile, nullptr);
                for (auto& mat : model.materials)
                {
                    if (!mat.diffuseTexturePath.empty())
                    {
                        auto texPath = fullPath.parent_path() / mat.diffuseTexturePath;
                        mat.diffuseTexture = graphicsDevice->createTexture2D(texPath.string());
                    }
                }
            }
        }

        inline void handleUseMaterial(std::istringstream& iss, ModelData& model, Material& currentMaterial)
        {
            std::string matName;
            iss >> matName;

            if (model.materialGroups.empty())
            {
                model.materialGroups.push_back({ "default", 0u,
                    static_cast<unsigned>(model.indices.size()), -1 });
            }
            else
            {
                model.materialGroups.back().indexCount =
                    static_cast<unsigned>(model.indices.size()) - model.materialGroups.back().startIndex;
            }

            auto it = std::find_if(model.materials.begin(), model.materials.end(),
                [&](const Material& m) { return m.name == matName; });

            if (it != model.materials.end())
            {
                int matIndex = static_cast<int>(std::distance(model.materials.begin(), it));
                model.materialGroups.push_back({ matName,
                    static_cast<unsigned>(model.indices.size()), 0, matIndex });

                currentMaterial = *it;
            }
        }

        inline void handleFaceLine(
            std::istringstream& iss,
            const std::vector<XMFLOAT3>& positions,
            const std::vector<XMFLOAT3>& normals,
            const std::vector<XMFLOAT2>& texcoords,
            std::unordered_map<VertexKey, uint32_t, VertexKeyHasher>& vertexLookup,
            const Material& currentMaterial,
            ModelData& model,
            XMFLOAT3& bbMin, XMFLOAT3& bbMax)
        {
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token) tokens.push_back(token);
            if (tokens.size() < 3) return;

            for (size_t i = 1; i + 1 < tokens.size(); ++i)
            {
                std::array<std::string, 3> tri = { tokens[0], tokens[i], tokens[i + 1] };
                for (int j = 0; j < 3; ++j)
                {
                    unsigned vi = 0, ti = 0, ni = 0;
                    parseFaceToken(tri[j], vi, ti, ni);

                    VertexKey key{};
                    key.position = (vi > 0 && vi <= positions.size()) ? positions[vi - 1] : XMFLOAT3(0.0f, 0.0f, 0.0f);
                    key.texcoord = (ti > 0 && ti <= texcoords.size()) ? texcoords[ti - 1] : XMFLOAT2(0.0f, 0.0f);
                    key.normal = (ni > 0 && ni <= normals.size()) ? normals[ni - 1] : XMFLOAT3(0.0f, 0.0f, 0.0f);

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

                        XMFLOAT4 color(
                            currentMaterial.diffuseColor.x,
                            currentMaterial.diffuseColor.y,
                            currentMaterial.diffuseColor.z,
                            1.0f
                        );

                        model.vertices.emplace_back(key.position, key.normal, key.texcoord, color);
                        model.indices.push_back(newIndex);

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

    namespace {
        const char BINARY_SIGNATURE[4] = { 'D', 'X', '3', 'B' };
        const uint32_t BINARY_VERSION = 1;

        template<typename T>
        void writePOD(std::ofstream& out, const T& data) {
            out.write(reinterpret_cast<const char*>(&data), sizeof(T));
        }

        template<typename T>
        void readPOD(std::ifstream& in, T& data) {
            in.read(reinterpret_cast<char*>(&data), sizeof(T));
        }

        void writeString(std::ofstream& out, const std::string& str) {
            uint32_t len = static_cast<uint32_t>(str.size());
            writePOD(out, len);
            if (len > 0) out.write(str.data(), len);
        }

        void readString(std::ifstream& in, std::string& str) {
            uint32_t len = 0;
            readPOD(in, len);
            if (len > 0) {
                str.resize(len);
                in.read(str.data(), len);
            }
            else {
                str.clear();
            }
        }

        template<typename T>
        void writeVector(std::ofstream& out, const std::vector<T>& vec) {
            uint32_t size = static_cast<uint32_t>(vec.size());
            writePOD(out, size);
            if (size > 0) out.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
        }

        template<typename T>
        void readVector(std::ifstream& in, std::vector<T>& vec) {
            uint32_t size = 0;
            readPOD(in, size);
            vec.resize(size);
            if (size > 0) in.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
        }
    }

    bool ModelImporter::saveBinary(const std::string& path, const ModelData& data)
    {
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open()) {
            DX3D_LOG_ERROR("Failed to open file for binary saving: {}", path);
            return false;
        }

        // Header & Version
        out.write(BINARY_SIGNATURE, 4);
        writePOD(out, BINARY_VERSION);

        // Global Properties
        writePOD(out, data.boundingBox);
        writePOD(out, data.hasNormals);
        writePOD(out, data.hasTexcoords);
        writeString(out, data.sourcePath);

        // Geometry (Vertices & Indices)
        writeVector(out, data.vertices);
        writeVector(out, data.indices);

        // Materials
        // We cannot write the struct directly because of std::string and pointers
        uint32_t matCount = static_cast<uint32_t>(data.materials.size());
        writePOD(out, matCount);

        for (const auto& mat : data.materials) {
            writeString(out, mat.name);

            // Write POD fields manually to avoid padding/pointer issues
            writePOD(out, mat.diffuseColor);
            writePOD(out, mat.shininess);
            writePOD(out, mat.ambientColor);
            writePOD(out, mat.opacity);
            writePOD(out, mat.specularColor);

            writeString(out, mat.diffuseTexturePath);
        }

        // Material Groups
        uint32_t groupCount = static_cast<uint32_t>(data.materialGroups.size());
        writePOD(out, groupCount);
        for (const auto& grp : data.materialGroups) {
            writeString(out, grp.name);
            writePOD(out, grp.startIndex);
            writePOD(out, grp.indexCount);
            writePOD(out, grp.materialIndex);
        }

        // Material Names List
        uint32_t nameCount = static_cast<uint32_t>(data.materialNames.size());
        writePOD(out, nameCount);
        for (const auto& name : data.materialNames) {
            writeString(out, name);
        }

        DX3D_LOG_INFO("Saved binary model cache: {}", path);
        return true;
    }

    ModelData ModelImporter::loadBinary(const std::string& path)
    {
        ModelData data;
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) return data; // Return empty

        // Header & Version Check
        char sig[4];
        in.read(sig, 4);
        uint32_t ver = 0;
        readPOD(in, ver);

        if (strncmp(sig, BINARY_SIGNATURE, 4) != 0 || ver != BINARY_VERSION) {
            DX3D_LOG_WARNING("Binary model version mismatch or invalid format: {}", path);
            return data; // Return empty to trigger fallback
        }

        // Global Properties
        readPOD(in, data.boundingBox);
        readPOD(in, data.hasNormals);
        readPOD(in, data.hasTexcoords);
        readString(in, data.sourcePath);

        // Geometry
        readVector(in, data.vertices);
        readVector(in, data.indices);

        // Materials
        uint32_t matCount = 0;
        readPOD(in, matCount);
        data.materials.reserve(matCount);

        for (uint32_t i = 0; i < matCount; ++i) {
            Material mat;
            readString(in, mat.name);

            readPOD(in, mat.diffuseColor);
            readPOD(in, mat.shininess);
            readPOD(in, mat.ambientColor);
            readPOD(in, mat.opacity);
            readPOD(in, mat.specularColor);

            readString(in, mat.diffuseTexturePath);

            // Re-link texture if AssetManager is available
            if (m_assets && !mat.diffuseTexturePath.empty()) {
                // Construct the full path for the texture relative to the binary file (or assets root)
                std::filesystem::path binPath(path);
                std::filesystem::path texPath = binPath.parent_path() / mat.diffuseTexturePath;

                // Use AssetManager to get/cache the texture
                mat.diffuseTexture = m_assets->getTexture(texPath.string());
            }

            data.materials.push_back(mat);
        }

        // Material Groups
        uint32_t groupCount = 0;
        readPOD(in, groupCount);
        data.materialGroups.reserve(groupCount);
        for (uint32_t i = 0; i < groupCount; ++i) {
            MaterialGroup grp;
            readString(in, grp.name);
            readPOD(in, grp.startIndex);
            readPOD(in, grp.indexCount);
            readPOD(in, grp.materialIndex);
            data.materialGroups.push_back(grp);
        }

        // Material Names
        uint32_t nameCount = 0;
        readPOD(in, nameCount);
        data.materialNames.reserve(nameCount);
        for (uint32_t i = 0; i < nameCount; ++i) {
            std::string name;
            readString(in, name);
            data.materialNames.push_back(name);
        }

        DX3D_LOG_INFO("Loaded binary model: {} ({} verts)", path, data.vertices.size());
        return data;
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

        std::vector<XMFLOAT3> positions, normals;
        std::vector<XMFLOAT2> texcoords;
        std::unordered_map<VertexKey, uint32_t, VertexKeyHasher> vertexLookup;
        Material currentMaterial;

        XMFLOAT3 bbMin{ FLT_MAX,  FLT_MAX,  FLT_MAX };
        XMFLOAT3 bbMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string prefix; iss >> prefix;

            if (prefix == "v")      parseVertexLine(iss, positions);
            else if (prefix == "vn") parseNormalLine(iss, normals);
            else if (prefix == "vt") parseTexcoordLine(iss, texcoords);
            else if (prefix == "mtllib") handleMaterialLib(iss, fullPath, model, m_graphicsDevice, m_assets);
            else if (prefix == "usemtl") handleUseMaterial(iss, model, currentMaterial);
            else if (prefix == "f") handleFaceLine(
                iss, positions, normals, texcoords,
                vertexLookup, currentMaterial, model, bbMin, bbMax);
        }
        file.close();

        if (!model.materialGroups.empty())
        {
            model.materialGroups.back().indexCount =
                static_cast<unsigned>(model.indices.size()) - model.materialGroups.back().startIndex;
        }

        if (!model.vertices.empty())
            model.boundingBox = { bbMin, bbMax };

        // Generate smooth normals if missing
        if (!model.hasNormals && !model.indices.empty())
        {
            DX3D_LOG_WARNING("Model has no normals, generating smooth normals...");

            for (auto& v : model.vertices)
            {
                v.normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }

            for (size_t i = 0; i < model.indices.size(); i += 3)
            {
                uint32_t i0 = model.indices[i + 0];
                uint32_t i1 = model.indices[i + 1];
                uint32_t i2 = model.indices[i + 2];

                const XMFLOAT3& p0 = model.vertices[i0].position;
                const XMFLOAT3& p1 = model.vertices[i1].position;
                const XMFLOAT3& p2 = model.vertices[i2].position;

                XMVECTOR v0 = XMLoadFloat3(&p0);
                XMVECTOR v1 = XMLoadFloat3(&p1);
                XMVECTOR v2 = XMLoadFloat3(&p2);

                XMVECTOR e1 = XMVectorSubtract(v1, v0);
                XMVECTOR e2 = XMVectorSubtract(v2, v0);

                XMVECTOR n = XMVector3Cross(e1, e2);
                n = XMVector3Normalize(n);

                // accumulate normals
                {
                    XMVECTOR n0 = XMLoadFloat3(&model.vertices[i0].normal);
                    n0 = XMVectorAdd(n0, n);
                    XMStoreFloat3(&model.vertices[i0].normal, n0);
                }
                {
                    XMVECTOR n1 = XMLoadFloat3(&model.vertices[i1].normal);
                    n1 = XMVectorAdd(n1, n);
                    XMStoreFloat3(&model.vertices[i1].normal, n1);
                }
                {
                    XMVECTOR n2 = XMLoadFloat3(&model.vertices[i2].normal);
                    n2 = XMVectorAdd(n2, n);
                    XMStoreFloat3(&model.vertices[i2].normal, n2);
                }
            }

            for (auto& v : model.vertices)
            {
                XMVECTOR nv = XMLoadFloat3(&v.normal);
                nv = XMVector3Normalize(nv);
                XMStoreFloat3(&v.normal, nv);
            }

            model.hasNormals = true;
        }

        DX3D_LOG_INFO(
            "Loaded {} vertices, {} indices ({} tris) from {} | Normals:{} UVs:{}",
            model.vertices.size(),
            model.indices.size(),
            model.indices.size() / 3,
            fullPath.string(),
            model.hasNormals ? "Y" : "N",
            model.hasTexcoords ? "Y" : "N"
        );

        return model;
    }
}
