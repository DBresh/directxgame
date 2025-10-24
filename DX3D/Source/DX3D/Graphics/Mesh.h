#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Math/Vertex.h>
#include <vector>

namespace dx3d
{

    class Mesh final : public GraphicsResource
    {
    public:
        Mesh(const std::vector<Vertex>& vertices,
            const std::vector<unsigned int>& indices,
            const GraphicsResourceDesc& gDesc);

        void draw(DeviceContext& context) const;

        const VertexBuffer& getVertexBuffer() const noexcept { return *m_vertexBuffer; }
        const IndexBuffer& getIndexBuffer() const noexcept { return *m_indexBuffer; }
        unsigned int getIndexCount() const noexcept { return m_indexCount; }

    private:
        VertexBufferPtr m_vertexBuffer{};
        IndexBufferPtr m_indexBuffer{};
        unsigned int m_indexCount{ 0 };
    };

}