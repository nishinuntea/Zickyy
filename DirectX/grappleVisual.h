#pragma once

#include "renderer.h"
#include "gameObject.h"

class GrappleVisual final : public GameObject
{
private:
	static constexpr UINT MaxVertexCount = 256;

	ID3D11Buffer* m_VertexBuffer{};
	ID3D11InputLayout* m_VertexLayout{};
	ID3D11VertexShader* m_VertexShader{};
	ID3D11PixelShader* m_PixelShader{};

	float m_CaptionUpdateTime{};

	void DrawVertices(const std::vector<VERTEX_3D>& Vertices, bool ScreenSpace);

public:
	void Init() override;
	void Uninit() override;
	void Update(float DeltaTime) override;
	void Draw() override;
};
