#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Math/Vec3d.h>
#include <Windows.h>

#include <DX3D/Graphics/GraphicsLogUtils.h>

using namespace DirectX;

namespace dx3d
{
	Camera::Camera()
	{
		updateViewMatrix();
		setPerspective(degToRad(90.0f), 16.0f / 9.0f, 0.1f, 100000.0f);
	}

	void Camera::setPerspective(float fov, float aspect, float zNear, float zFar)
	{
		XMMATRIX P = XMMatrixPerspectiveFovLH(fov, aspect, zNear, zFar);
		XMStoreFloat4x4(&m_proj, P);
	}

	void Camera::setScreenSize(float width, float height)
	{
		if (height <= 0.0f) return;

		float aspect = width / height;

		setPerspective(degToRad(90.0f), aspect, 0.1f, 100000.0f);
	}

	void Camera::updateViewMatrix()
	{
		float cosPitch = cosf(m_pitch);
		float sinPitch = sinf(m_pitch);
		float cosYaw = cosf(m_yaw);
		float sinYaw = sinf(m_yaw);

		XMVECTOR forward = XMVectorSet(
			cosPitch * sinYaw,
			sinPitch,
			-cosPitch * cosYaw,
			0.0f
		);
		forward = XMVector3Normalize(forward);
		XMStoreFloat3(&m_forward, forward);

		if (m_isOrbiting)
		{
			Vec3d fwdDouble(m_forward.x, m_forward.y, m_forward.z);
			m_position = m_orbitTarget - (fwdDouble * m_orbitDistance);
		}

		XMVECTOR eye = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);

		XMMATRIX V = XMMatrixLookAtLH(eye, eye + forward, up);
		XMStoreFloat4x4(&m_view, V);
	}

	void Camera::update()
	{
		if (m_isOrbiting) return;

		double dt = (float)Time::Instance()->deltaTime();
		double move = m_speed * dt;

		Vec3d fwd(m_forward.x, m_forward.y, m_forward.z);
		Vec3d up(0.0, 1.0, 0.0);
		Vec3d right = Vec3d::Cross(up, fwd).normalized();

		m_position += fwd * (m_moveForward * move);
		m_position += right * (m_moveRight * move);
		m_position += up * (m_moveUp * move);

		updateViewMatrix();
	}

	void Camera::screenPointToRay(int mouseX, int mouseY, int screenW, int screenH, XMVECTOR& outOrigin, XMVECTOR& outDir) const
	{
		if (screenW == 0 || screenH == 0) return;

		float vx = (2.0f * mouseX / screenW - 1.0f);
		float vy = (-2.0f * mouseY / screenH + 1.0f);

		XMMATRIX P = XMLoadFloat4x4(&m_proj);
		vx /= XMVectorGetX(P.r[0]);
		vy /= XMVectorGetY(P.r[1]);

		XMMATRIX V = XMLoadFloat4x4(&m_view);
		XMMATRIX invV = XMMatrixInverse(nullptr, V);

		outOrigin = invV.r[3];

		XMVECTOR dir = XMVectorSet(
			vx * XMVectorGetX(invV.r[0]) + vy * XMVectorGetX(invV.r[1]) + XMVectorGetX(invV.r[2]),
			vx * XMVectorGetY(invV.r[0]) + vy * XMVectorGetY(invV.r[1]) + XMVectorGetY(invV.r[2]),
			vx * XMVectorGetZ(invV.r[0]) + vy * XMVectorGetZ(invV.r[1]) + XMVectorGetZ(invV.r[2]),
			0.0f
		);

		outDir = XMVector3Normalize(dir);
	}

	void Camera::onKeyDown(int key)
	{
		if (key == VK_SHIFT)
			m_speed *= 5;
	}

	void Camera::onKeyUp(int key)
	{
		if ((char)key == 'W' || (char)key == 'S') m_moveForward = 0;
		if ((char)key == 'A' || (char)key == 'D') m_moveRight = 0;
		if (key == VK_SPACE || key == VK_CONTROL) m_moveUp = 0;

		if (key == VK_SHIFT)
			m_speed /= 5;
	}

	void Camera::onKeyPress(int key)
	{
		float dt = (float)Time::Instance()->deltaTime();

		if ((char)key == 'W') m_moveForward = 1;
		if ((char)key == 'S') m_moveForward = -1;

		if ((char)key == 'D') m_moveRight = 1;
		if ((char)key == 'A') m_moveRight = -1;

		if (key == VK_SPACE)   m_moveUp = 1;
		if (key == VK_CONTROL) m_moveUp = -1;

		if ((char)key == 'I') m_pitch += m_sensitivity * 10 * dt;
		if ((char)key == 'K') m_pitch -= m_sensitivity * 10 * dt;
		if ((char)key == 'J') m_yaw += m_sensitivity * 10 * dt;
		if ((char)key == 'L') m_yaw -= m_sensitivity * 10 * dt;
	}

	void Camera::onMouseMove(Point deltaMouse)
	{
		MouseState ms = InputSystem::get()->getMouseState();

		if (ms.rightButton) {
			m_yaw -= deltaMouse.x * m_sensitivity * 0.01f;
			m_pitch -= deltaMouse.y * m_sensitivity * 0.01f;
		}

		const float limit = 1.55f;
		if (m_pitch > limit) m_pitch = limit;
		if (m_pitch < -limit) m_pitch = -limit;

		updateViewMatrix();
	}

	void Camera::onMouseDown(int) {}
	void Camera::onMouseUp(int) {}
	

	void Camera::onMouseWheel(int delta)
	{
		if (InputSystem::get()->isMouseKeyDown(1))
		{
			m_speed += delta;
			return;
		}
		if (m_isOrbiting)
		{
			m_orbitDistance -= static_cast<double>(delta * m_speed) * 0.01;
			if (m_orbitDistance < 1.0) m_orbitDistance = 1.0;
			updateViewMatrix();
		}
	}

	void Camera::setOrbitMode(bool enabled)
	{
		m_isOrbiting = enabled;
		if (enabled)
		{
			Vec3d diff = m_position - m_orbitTarget;
			m_orbitDistance = diff.magnitude();

			if (m_orbitDistance < 2.0) m_orbitDistance = 10.0;
		}
		updateViewMatrix();
	}

	void Camera::setOrbitTarget(const Vec3d& target)
	{
		m_orbitTarget = target;
		if (m_isOrbiting) updateViewMatrix();
	}
}
