#pragma once

#include "gameObject.h"

class Box final : public GameObject
{
private:
	ID3D11InputLayout* m_VertexLayout{};
	ID3D11VertexShader* m_VertexShader{};
	ID3D11PixelShader* m_PixelShader{};

public:
	void Init() override;
	void Uninit() override;
	void Update(float DeltaTime) override;
	void Draw() override;

	Vector3 GetCollisionMin()
	{
		return {
			m_Position.x - m_Scale.x,
			m_Position.y,
			m_Position.z - m_Scale.z
		};
	}

	Vector3 GetCollisionMax()
	{
		return {
			m_Position.x + m_Scale.x,
			m_Position.y + m_Scale.y * 2.0f,
			m_Position.z + m_Scale.z
		};
	}
};
