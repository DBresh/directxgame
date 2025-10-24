#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Math/Vertex.h>

namespace dx3d
{

    Mesh::Mesh(const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        const GraphicsResourceDesc& gDesc) :
        GraphicsResource(gDesc),
        m_indexCount(static_cast<unsigned int>(indices.size()))
    {
        if (vertices.empty()) DX3D_LOG_THROW_ERROR("Vertex list is empty.");
        if (indices.empty()) DX3D_LOG_THROW_ERROR("Index list is empty.");

        VertexBufferDesc vbDesc{};
        vbDesc.vertexList = vertices.data();
        vbDesc.vertexListSize = static_cast<unsigned int>(vertices.size());
        vbDesc.vertexSize = sizeof(Vertex);

        auto graphicsDevice = std::const_pointer_cast<GraphicsDevice>(m_graphicsDevice);
        m_vertexBuffer = graphicsDevice->createVertexBuffer(vbDesc);
 
        IndexBufferDesc ibDesc{};
        ibDesc.indexList = indices.data();
        ibDesc.indexCount = static_cast<unsigned int>(indices.size());
        ibDesc.use32Bit = true;

        m_indexBuffer = graphicsDevice->createIndexBuffer(ibDesc);
    }

    void Mesh::draw(DeviceContext& context) const
    {
        context.setVertexBuffer(*m_vertexBuffer);
        context.setIndexBuffer(*m_indexBuffer);
        context.drawIndexedTriangleList(m_indexCount, 0u, 0u);
    }

}