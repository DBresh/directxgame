#pragma once
#include <DirectXMath.h>
#include <DX3D/Core/Time.h>
#include <DX3D/InputSystem/InputListener.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Math/Vec3d.h>

namespace dx3d
{
	using namespace DirectX;

	class Camera final : public InputListener
	{
	public:
		Camera();

		void update();

		const XMFLOAT4X4& getViewMatrix() const noexcept { return m_view; }
		const XMFLOAT4X4& getProjectionMatrix() const noexcept { return m_proj; }

		void setPerspective(float fov, float aspect, float zNear, float zFar);
		
		void setPosition(const dx3d::Vec3d& pos) { m_position = pos; updateViewMatrix(); }
		void setPosition(double x, double y, double z) { m_position = dx3d::Vec3d(x, y, z); updateViewMatrix(); }
		dx3d::Vec3d getPosition() const noexcept { return m_position; }

		void setScreenSize(float width, float height);

		void onKeyDown(int key) override;
		void onKeyUp(int key) override;
		void onKeyPress(int key) override;
		void onMouseMove(Point deltaMouse) override;
		void onMouseDown(int button) override;
		void onMouseUp(int button) override;
		void onMouseWheel(int delta) override;

		void setOrbitTarget(const Vec3d& target);
		void setOrbitMode(bool enabled);
		bool isOrbiting() const noexcept { return m_isOrbiting; }

		void screenPointToRay(int mouseX, int mouseY, int screenW, int screenH, DirectX::XMVECTOR& outOrigin, DirectX::XMVECTOR& outDir) const;

		static constexpr float degToRad(float deg) { return deg * 3.1415926535f / 180.0f; }

	public:
		float m_speed{ 2.0f };

	private:
		void updateViewMatrix();

	private:
		Vec3d m_position{ 0, 0, 0 };
		XMFLOAT3 m_forward{ 0, 0, 1 };

		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_proj;

		float m_yaw{ 0.0f };
		float m_pitch{ 0.0f };
		float m_sensitivity{ 0.2f };

		int m_moveForward{ 0 };
		int m_moveRight{ 0 };
		int m_moveUp{ 0 };

		bool m_isOrbiting{ false };
		Vec3d m_orbitTarget{ 0.0f, 0.0f, 0.0f };
		double m_orbitDistance{ 15.0f };
	};
}
