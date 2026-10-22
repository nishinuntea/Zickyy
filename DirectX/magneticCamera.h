#pragma once

#include "gameObject.h"

class MagneticCamera final : public GameObject
{
private:
	Vector3 m_Target{};
	Vector3 m_FinalTarget{};
	XMMATRIX m_ViewMatrix = XMMatrixIdentity();
	float m_Yaw{};
	float m_Pitch{ 0.32f };

public:
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	Vector3 GetForward() override;
	Vector3 GetRight() override;
	Vector3 GetFlatForward() const;
	Vector3 GetFlatRight() const;
};
