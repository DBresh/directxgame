#pragma once
#include <Game/Kepler/OrbitData.h>
#include <Game/Kepler/KeplerPhysics.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Graphics/Core/DeviceContext.h>
#include <DX3D/Graphics/Buffers/VertexBuffer.h>
#include <DX3D/Graphics/Buffers/ConstantBuffer.h>
#include <DX3D/Math/Vertex.h>
#include <vector>
#include <memory>
#include <d3d11.h>

namespace Simulator
{
    class OrbitVisualizer
    {
    public:
        OrbitVisualizer() = default;
        ~OrbitVisualizer() = default;

        void init(dx3d::GraphicsDevice& device)
        {
            m_constantBuffer = device.createConstantBuffer({ nullptr, sizeof(DirectX::XMFLOAT4X4) * 3 });
        }

        void update(dx3d::GraphicsDevice& device, OrbitData& orbitData, const Vec3d& attractorOrigin)
        {
            if (!orbitData.isPathDirty) return;

            const int numPoints = 150;
            std::vector<Vec3d> rawPoints;

            Kepler::GetOrbitPoints(orbitData, rawPoints, numPoints, attractorOrigin, 1500.0);

            m_vertexCount = static_cast<uint32_t>(rawPoints.size());
            if (m_vertexCount == 0) return;

            std::vector<dx3d::Vertex> vertices(m_vertexCount);
            for (uint32_t i = 0; i < m_vertexCount; ++i)
            {
                vertices[i].position = DirectX::XMFLOAT3(
                    static_cast<float>(rawPoints[i].x),
                    static_cast<float>(rawPoints[i].y),
                    static_cast<float>(rawPoints[i].z)
                );
                vertices[i].color = DirectX::XMFLOAT4(0.2f, 0.8f, 1.0f, 1.0f);
            }

            m_vertexBuffer = device.createVertexBuffer({ vertices.data(), sizeof(dx3d::Vertex), m_vertexCount });

            orbitData.isPathDirty = false;
        }

        void draw(dx3d::DeviceContext& context, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj)
        {
            if (!m_vertexBuffer || m_vertexCount == 0 || !m_constantBuffer) return;

            DirectX::XMMATRIX w = DirectX::XMMatrixIdentity();
            DirectX::XMMATRIX v = DirectX::XMLoadFloat4x4(&view);
            DirectX::XMMATRIX p = DirectX::XMLoadFloat4x4(&proj);

            DirectX::XMFLOAT4X4 matrices[3];
            DirectX::XMStoreFloat4x4(&matrices[0], DirectX::XMMatrixTranspose(w));
            DirectX::XMStoreFloat4x4(&matrices[1], DirectX::XMMatrixTranspose(v));
            DirectX::XMStoreFloat4x4(&matrices[2], DirectX::XMMatrixTranspose(p));
            
            context.updateConstantBuffer(*m_constantBuffer, matrices, sizeof(DirectX::XMFLOAT4X4) * 3);
            context.setVSConstantBuffer(*m_constantBuffer, 0);

            auto d3dContext = context.getD3D11Context();
            d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

            UINT stride = sizeof(dx3d::Vertex);
            UINT offset = 0;
            ID3D11Buffer* rawBuffer = m_vertexBuffer->get();
            d3dContext->IASetVertexBuffers(0, 1, &rawBuffer, &stride, &offset);

            d3dContext->Draw(m_vertexCount, 0);

            d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }

    private:
        std::shared_ptr<dx3d::VertexBuffer> m_vertexBuffer;
        std::shared_ptr<dx3d::ConstantBuffer> m_constantBuffer;
        uint32_t m_vertexCount = 0;
    };
}