#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "camera.h"
#include "player.h"

#include <algorithm>
#include <cmath>

void Camera::Init()
{
	m_Layer = 0;
	m_Target = { 0.0f, 1.5f, -24.0f };
	m_LookTarget = m_Target;
	m_FinalTarget = m_Target;
	m_Position = { 0.0f, 7.0f, -36.0f };
}

void Camera::Uninit()
{
	m_Position = {};
	m_Target = {};
	m_LookTarget = {};
	m_FinalTarget = {};
}

void Camera::Update(float DeltaTime)
{
	Player* player = Manager::GetGameObject<Player>();
	if (player == nullptr)
	{
		return;
	}

	constexpr float MouseSensitivityX = 0.0026f;
	constexpr float MouseSensitivityY = 0.0022f;
	if (Input::IsMouseLookActive())
	{
		m_Yaw += Input::GetMouseDeltaX() * MouseSensitivityX;
		m_Pitch += Input::GetMouseDeltaY() * MouseSensitivityY;
	}

	if (Input::GetKeyPress(VK_RIGHT))
	{
		m_Yaw += 2.4f * DeltaTime;
	}
	if (Input::GetKeyPress(VK_LEFT))
	{
		m_Yaw -= 2.4f * DeltaTime;
	}
	if (Input::GetKeyPress(VK_UP))
	{
		m_Pitch -= 1.2f * DeltaTime;
	}
	if (Input::GetKeyPress(VK_DOWN))
	{
		m_Pitch += 1.2f * DeltaTime;
	}
	m_Pitch = std::clamp(m_Pitch, 0.12f, 0.95f);

	const float speed = player->GetSpeed();
	const float speedRatio = std::clamp(speed / 60.0f, 0.0f, 1.0f);
	const Vector3 desiredTarget =
		player->GetPosition() +
		Vector3(0.0f, 1.5f, 0.0f) +
		player->GetVelocity() * 0.055f;
	const float followResponse = 1.0f - expf(-8.0f * DeltaTime);
	m_Target += (desiredTarget - m_Target) * followResponse;

	const float cameraDistance = 12.0f + speedRatio * 5.0f;
	const float horizontalDistance = cosf(m_Pitch) * cameraDistance;
	const float verticalDistance = sinf(m_Pitch) * cameraDistance;
	m_Position = m_Target + Vector3(
		-sinf(m_Yaw) * horizontalDistance,
		verticalDistance,
		-cosf(m_Yaw) * horizontalDistance);

	Vector3 desiredLookTarget = m_Target;
	if (player->IsGrappling())
	{
		constexpr float ReleaseLockDistance = 15.0f;
		constexpr float FullLockDistance = 18.0f;

		const Vector3 grapplePoint = player->GetGrapplePoint();
		const Vector3 playerCenter =
			player->GetPosition() + Vector3(0.0f, 1.0f, 0.0f);
		const float grappleDistance =
			(grapplePoint - playerCenter).length();
		const float normalizedDistance = std::clamp(
			(grappleDistance - ReleaseLockDistance) /
				(FullLockDistance - ReleaseLockDistance),
			0.0f,
			1.0f);
		const float anchorLockWeight =
			normalizedDistance * normalizedDistance *
			(3.0f - 2.0f * normalizedDistance);

		desiredLookTarget +=
			(grapplePoint - desiredLookTarget) * anchorLockWeight;
	}
	const float lookResponseSpeed =
		player->IsGrappling() ? 12.0f : 9.0f;
	const float lookResponse =
		1.0f - expf(-lookResponseSpeed * DeltaTime);
	m_LookTarget +=
		(desiredLookTarget - m_LookTarget) * lookResponse;

	m_ShakeTime += DeltaTime;
	const Vector3 shakeOffset =
		m_Shake * cosf(m_ShakeTime * 80.0f);
	m_Shake *= expf(-12.0f * DeltaTime);
	m_FinalTarget = m_LookTarget + shakeOffset;

	const float desiredFieldOfView = 1.0f + speedRatio * 0.3f;
	const float fovResponse = 1.0f - expf(-6.0f * DeltaTime);
	m_FieldOfView +=
		(desiredFieldOfView - m_FieldOfView) * fovResponse;
}

void Camera::Draw()
{
	const XMMATRIX projection = XMMatrixPerspectiveFovLH(
		m_FieldOfView,
		static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT,
		0.3f,
		1000.0f);
	Renderer::SetProjectionMatrix(projection);

	const XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };
	m_ViewMatrix = XMMatrixLookAtLH(
		XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(&m_Position)),
		XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(&m_FinalTarget)),
		XMLoadFloat3(&up));
	Renderer::SetViewMatrix(m_ViewMatrix);
}

void Camera::Shake(Vector3 ShakeAmount)
{
	m_Shake = ShakeAmount;
	m_ShakeTime = 0.0f;
}
