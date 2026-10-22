#pragma once

#include "gameObject.h"

class Camera final : public GameObject
{
private:
	Vector3 m_Target{};
	Vector3 m_LookTarget{};
	Vector3 m_FinalTarget{};
	XMMATRIX m_ViewMatrix = XMMatrixIdentity();

	Vector3 m_Shake{};
	float m_ShakeTime{};
	float m_Yaw{};
	float m_Pitch{ 0.38f };
	float m_FieldOfView{ 1.0f };

public:
	void Init() override;
	void Uninit() override;
	void Update(float DeltaTime) override;
	void Draw() override;

	void Shake(Vector3 ShakeAmount);

	XMMATRIX GetViewMatrix() const { return m_ViewMatrix; }

	Vector3 GetForward() override
	{
		Vector3 forward = m_FinalTarget - m_Position;
		forward.normalize();
		return forward;
	}

	Vector3 GetRight() override
	{
		const Vector3 forward = GetForward();
		const Vector3 up = { 0.0f, 1.0f, 0.0f };
		Vector3 right = Vector3::cross(up, forward);
		right.normalize();
		return right;
	}
};
