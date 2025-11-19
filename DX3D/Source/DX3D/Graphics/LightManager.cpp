#include <DX3D/Graphics/LightManager.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DepthTexture2D.h>
#include <DX3D/Core/Logger.h>

using namespace DirectX;

namespace dx3d
{
	void LightManager::clear()
	{
		m_lights.clear();
	}

	void LightManager::addDirectional(const XMFLOAT3& dir,
		const XMFLOAT3& color,
		float intensity,
		bool shadows)
	{
		Light l{};
		l.type = LightType::Directional;

		XMVECTOR d = XMLoadFloat3(&dir);
		d = XMVector3Normalize(d);
		XMStoreFloat3(&l.direction, d);

		l.color = color;
		l.intensity = intensity;
		l.castShadows = shadows;

		m_lights.push_back(l);
	}

	void LightManager::addPoint(const XMFLOAT3& pos,
		const XMFLOAT3& color,
		float range,
		float intensity)
	{
		Light l{};
		l.type = LightType::Point;

		l.position = pos;
		l.color = color;
		l.range = range;
		l.intensity = intensity;

		m_lights.push_back(l);
	}

	static void LogMatrix(const char* name, DirectX::CXMMATRIX M)
	{
		DirectX::XMFLOAT4X4 m;
		DirectX::XMStoreFloat4x4(&m, M);
		DX3D_LOG_INFO(
			"Matrix Log: {} [as Row-Major]\n"
			"  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
			"  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
			"  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
			"  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]",
			name,
			m._11, m._12, m._13, m._14,
			m._21, m._22, m._23, m._24,
			m._31, m._32, m._33, m._34,
			m._41, m._42, m._43, m._44
		);
	}

	void LightManager::addSpot(const XMFLOAT3& pos,
		const XMFLOAT3& dir,
		float angleDegrees,
		const XMFLOAT3& color,
		float range,
		float intensity,
		bool shadows)
	{
		Light l{};
		l.type = LightType::Spot;

		l.position = pos;

		XMVECTOR d = XMLoadFloat3(&dir);
		d = XMVector3Normalize(d);
		XMStoreFloat3(&l.direction, d);

		l.color = color;
		l.range = range;
		l.intensity = intensity;
		l.spotAngle = XMConvertToRadians(angleDegrees * 0.5f);

		DX3D_LOG_INFO("spot angle {}", l.spotAngle);

		l.castShadows = shadows;
		if (shadows)
			createSpotShadow(l);

		m_lights.push_back(std::move(l));
	}

	void LightManager::createSpotShadow(Light& l)
	{
		const UINT shadowSize = 2048;

		if (!l.shadow)
			l.shadow = std::make_shared<LightShadowData>();

		// Створюємо / оновлюємо depth-текстуру
		l.shadow->shadowMap = m_device->createDepthTexture2D(shadowSize, shadowSize);

		// --- Обчислюємо view матрицю (row-major на CPU) ---
		XMVECTOR pos = XMLoadFloat3(&l.position);
		XMVECTOR forward = XMLoadFloat3(&l.direction); // l.direction вже нормалізований
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);

		{
			XMFLOAT3 d3;
			XMStoreFloat3(&d3, forward);
			if (fabsf(d3.y) > 0.99f)
				up = XMVectorSet(0, 0, 1, 0);
		}

		XMMATRIX V = XMMatrixLookAtLH(pos, pos + forward, up);

		// --- FOV: у l.spotAngle зберігаємо half-angle в радіанах ---
		float halfAngle = l.spotAngle;        // half-angle (rad)
		float fov = halfAngle * 2.0f;   // full vertical FOV

		float aspect = 1.0f;
		float znear = 0.1f;
		float zfar = l.range;

		XMMATRIX P = XMMatrixPerspectiveFovLH(fov, aspect, znear, zfar);
		XMMATRIX VP = V * P; // row-major матриця V*P

		// --- Зберігаємо row-major V та P для shadow pass ---
		XMStoreFloat4x4(&l.shadow->view, V);
		XMStoreFloat4x4(&l.shadow->proj, P);

		// --- А сюди кладемо ВЖЕ ТРАНСПОНОВАНИЙ VP (GPU-ready column-major) ---
		XMMATRIX VP_t = XMMatrixTranspose(VP);
		XMStoreFloat4x4(&l.shadow->viewProj, VP_t);

		// Біас для SampleCmp (потім за бажанням підкрутиш)
		l.shadow->bias = 0.0065f;

		XMFLOAT3 d3;
		XMStoreFloat3(&d3, forward);

		DX3D_LOG_INFO(
			"Created spot shadow: pos({:.2f},{:.2f},{:.2f}) dir({:.2f},{:.2f},{:.2f}) halfAngleDeg={:.2f} fullFovDeg={:.2f}",
			l.position.x, l.position.y, l.position.z,
			d3.x, d3.y, d3.z,
			XMConvertToDegrees(halfAngle),
			XMConvertToDegrees(fov)
		);
	}

	void LightManager::uploadToGPU()
	{
		if (m_lights.empty())
			return;

		std::vector<LightGPU> gpuLights;
		gpuLights.reserve(m_lights.size());

		for (auto& l : m_lights)
		{
			LightGPU g{};
			g.type = static_cast<int>(l.type);

			g.posRange = XMFLOAT4(l.position.x, l.position.y, l.position.z, l.range);
			g.dirSpot = XMFLOAT4(l.direction.x, l.direction.y, l.direction.z, l.spotAngle);
			g.colInt = XMFLOAT4(l.color.x, l.color.y, l.color.z, l.intensity);

			gpuLights.push_back(g);
		}

		m_gpuBuffer = m_device->createStructuredBuffer(gpuLights);
	}

	void LightManager::onKeyDown(int key) {}
	void LightManager::onKeyUp(int key) {
		if ((char)key == 'N') {
			for (int i = 0; i < m_lights.size(); i++)
			{
				m_lights[i].spotAngle += XMConvertToRadians(0.5f);
				createSpotShadow(m_lights[i]);
			}
		}
		if ((char)key == 'M') {
			for (int i = 0; i < m_lights.size(); i++)
			{
				m_lights[i].spotAngle -= XMConvertToRadians(0.5f);
				createSpotShadow(m_lights[i]);
			}
		}
		if ((char)key == 'Y') {
			for (int i = 0; i < m_lights.size(); i++)
			{
				m_lights[i].position.y += 1.0f;
				createSpotShadow(m_lights[i]);
			}
		}
		if ((char)key == 'H') {
			for (int i = 0; i < m_lights.size(); i++)
			{
				m_lights[i].position.y -= 1.0f;
				createSpotShadow(m_lights[i]);
			}
		}
	}
	void LightManager::onKeyPress(int key) {

	}
	void LightManager::onMouseMove(Point deltaMouse) {}
	void LightManager::onMouseDown(int button) {}
	void LightManager::onMouseUp(int button) {}
	void LightManager::onMouseWheel(int delta) {}

	void LightManager::bind(DeviceContext& context, unsigned slot)
	{
		if (!m_gpuBuffer) return;
		context.setStructuredBuffer(*m_gpuBuffer, slot);
	}
}
