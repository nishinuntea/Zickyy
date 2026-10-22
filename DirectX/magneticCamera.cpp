#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "magneticCamera.h"
#include "magneticPrototype.h"

#include <algorithm>
#include <cmath>

void MagneticCamera::Init()
{
	m_Layer = 0;
	m_Target = { 0.0f, 1.2f, 0.0f };
	m_FinalTarget = m_Target;
	m_Position = { 0.0f, 6.0f, -11.0f };
}

void MagneticCamera::Uninit()
{
	m_Target = {};
	m_FinalTarget = {};
	m_Position = {};
}

void MagneticCamera::Update(float deltaTime)
{
	MagneticPrototype* prototype = Manager::GetGameObject<MagneticPrototype>();
	if (prototype == nullptr)
	{
		return;
	}

	constexpr float mouseSensitivityX = 0.0017f;
	constexpr float mouseSensitivityY = 0.0014f;
	if (Input::IsMouseLookActive())
	{
		m_Yaw += Input::GetMouseDeltaX() * mouseSensitivityX;
		m_Pitch += Input::GetMouseDeltaY() * mouseSensitivityY;
	}
	if (Input::GetKeyPress(VK_LEFT) || Input::GetKeyPress('Q'))
	{
		m_Yaw -= 1.45f * deltaTime;
	}
	if (Input::GetKeyPress(VK_RIGHT) || Input::GetKeyPress('E'))
	{
		m_Yaw += 1.45f * deltaTime;
	}
	if (Input::GetKeyPress(VK_UP))
	{
		m_Pitch -= 0.85f * deltaTime;
	}
	if (Input::GetKeyPress(VK_DOWN))
	{
		m_Pitch += 0.85f * deltaTime;
	}
	m_Pitch = std::clamp(m_Pitch, 0.15f, 0.68f);

	const Vector3 playerPosition = prototype->GetPlayerPosition();
	const Vector3 playerVelocity = prototype->GetPlayerVelocity();
	const Vector3 desiredTarget =
		playerPosition + Vector3(0.0f, 0.9f, 0.0f) + playerVelocity * 0.035f;
	const float follow = 1.0f - std::exp(-7.5f * deltaTime);
	m_Target += (desiredTarget - m_Target) * follow;

	const float distance = prototype->GetCurrentFloor() == 2 ? 12.5f : 10.5f;
	const float horizontalDistance = std::cos(m_Pitch) * distance;
	const float verticalDistance = std::sin(m_Pitch) * distance;
	m_Position = m_Target + Vector3(
		-std::sin(m_Yaw) * horizontalDistance,
		verticalDistance,
		-std::cos(m_Yaw) * horizontalDistance);
	m_FinalTarget = m_Target;
}

void MagneticCamera::Draw()
{
	const XMMATRIX projection = XMMatrixPerspectiveFovLH(
		0.92f,
		static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),
		0.15f,
		500.0f);
	Renderer::SetProjectionMatrix(projection);

	const XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };
	m_ViewMatrix = XMMatrixLookAtLH(
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&m_Position)),
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&m_FinalTarget)),
		XMLoadFloat3(&up));
	Renderer::SetViewMatrix(m_ViewMatrix);
}

Vector3 MagneticCamera::GetForward()
{
	Vector3 forward = m_FinalTarget - m_Position;
	forward.normalize();
	return forward;
}

Vector3 MagneticCamera::GetRight()
{
	const Vector3 forward = GetForward();
	Vector3 right = Vector3::cross({ 0.0f, 1.0f, 0.0f }, forward);
	right.normalize();
	return right;
}

Vector3 MagneticCamera::GetFlatForward() const
{
	Vector3 forward = m_FinalTarget - m_Position;
	forward.y = 0.0f;
	forward.normalize();
	return forward;
}

Vector3 MagneticCamera::GetFlatRight() const
{
	Vector3 right = Vector3::cross({ 0.0f, 1.0f, 0.0f }, GetFlatForward());
	right.normalize();
	return right;
}
