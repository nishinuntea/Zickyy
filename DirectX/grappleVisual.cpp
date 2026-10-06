#include "main.h"
#include "renderer.h"
#include "manager.h"
#include "player.h"
#include "grappleAnchor.h"
#include "grappleVisual.h"

#include <cwchar>

namespace
{
	VERTEX_3D MakeLineVertex(const Vector3& Position, const XMFLOAT4& Color)
	{
		VERTEX_3D vertex{};
		vertex.Position = XMFLOAT3(Position.x, Position.y, Position.z);
		vertex.Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex.Diffuse = Color;
		vertex.TexCoord = XMFLOAT2(0.0f, 0.0f);
		return vertex;
	}

	void AddLine(
		std::vector<VERTEX_3D>& Vertices,
		const Vector3& Start,
		const Vector3& End,
		const XMFLOAT4& Color)
	{
		Vertices.push_back(MakeLineVertex(Start, Color));
		Vertices.push_back(MakeLineVertex(End, Color));
	}
}

void GrappleVisual::Init()
{
	m_Layer = 3;

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(VERTEX_3D) * MaxVertexCount;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ThrowIfFailed(
		Renderer::GetDevice()->CreateBuffer(&bufferDesc, nullptr, &m_VertexBuffer),
		"ID3D11Device::CreateBuffer(grapple visual)");

	Renderer::CreateVertexShader(
		&m_VertexShader,
		&m_VertexLayout,
		"shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(
		&m_PixelShader,
		"shader\\unlitTexturePS.cso");
}

void GrappleVisual::Uninit()
{
	SafeRelease(m_VertexBuffer);
	SafeRelease(m_VertexLayout);
	SafeRelease(m_VertexShader);
	SafeRelease(m_PixelShader);
}

void GrappleVisual::Update(float DeltaTime)
{
	m_CaptionUpdateTime -= DeltaTime;
	if (m_CaptionUpdateTime > 0.0f)
	{
		return;
	}

	m_CaptionUpdateTime = 0.2f;

	const Player* player = Manager::GetGameObject<Player>();
	if (player == nullptr)
	{
		return;
	}

	const wchar_t* grappleState = L"NO TARGET";
	if (player->IsGrappling())
	{
		grappleState = player->IsBoosting() ? L"BOOST" : L"GRAPPLE";
	}
	else if (player->HasGrappleTarget())
	{
		grappleState = L"TARGET";
	}

	wchar_t caption[256]{};
	swprintf_s(
		caption,
		L"WIRE HUNTER | Speed %.0f | %s | Mouse Look | Tab Cursor | Z/LMB Grapple | Shift Boost | WASD | R Reset",
		player->GetSpeed(),
		grappleState);
	SetWindowTextW(GetWindow(), caption);
}

void GrappleVisual::Draw()
{
	Player* player = Manager::GetGameObject<Player>();
	if (player == nullptr)
	{
		return;
	}

	std::vector<VERTEX_3D> worldVertices;
	const auto anchors = Manager::GetGameObjects<GrappleAnchor>();
	worldVertices.reserve(anchors.size() * 6 + 4);

	for (GrappleAnchor* anchor : anchors)
	{
		XMFLOAT4 color = { 0.1f, 0.55f, 1.0f, 1.0f };
		float radius = 0.65f;
		if (anchor->IsAttached())
		{
			color = { 0.1f, 1.0f, 0.65f, 1.0f };
			radius = 1.2f;
		}
		else if (anchor->IsHighlighted())
		{
			color = { 1.0f, 0.75f, 0.1f, 1.0f };
			radius = 1.0f;
		}

		const Vector3 position = anchor->GetPosition();
		AddLine(
			worldVertices,
			position + Vector3(-radius, 0.0f, 0.0f),
			position + Vector3(radius, 0.0f, 0.0f),
			color);
		AddLine(
			worldVertices,
			position + Vector3(0.0f, -radius, 0.0f),
			position + Vector3(0.0f, radius, 0.0f),
			color);
		AddLine(
			worldVertices,
			position + Vector3(0.0f, 0.0f, -radius),
			position + Vector3(0.0f, 0.0f, radius),
			color);
	}

	if (player->IsGrappling())
	{
		const Vector3 playerPosition = player->GetPosition();
		const Vector3 anchorPosition = player->GetGrapplePoint();
		const XMFLOAT4 cableColor = player->IsBoosting()
			? XMFLOAT4(1.0f, 0.8f, 0.15f, 1.0f)
			: XMFLOAT4(0.15f, 1.0f, 0.75f, 1.0f);

		AddLine(
			worldVertices,
			playerPosition + Vector3(-0.18f, 1.05f, 0.0f),
			anchorPosition + Vector3(-0.08f, 0.0f, 0.0f),
			cableColor);
		AddLine(
			worldVertices,
			playerPosition + Vector3(0.18f, 1.05f, 0.0f),
			anchorPosition + Vector3(0.08f, 0.0f, 0.0f),
			cableColor);
	}

	DrawVertices(worldVertices, false);

	XMFLOAT4 crosshairColor = { 0.55f, 0.55f, 0.55f, 1.0f };
	if (player->IsGrappling())
	{
		crosshairColor = { 0.1f, 1.0f, 0.65f, 1.0f };
	}
	else if (player->HasGrappleTarget())
	{
		crosshairColor = { 1.0f, 0.75f, 0.1f, 1.0f };
	}

	const float centerX = static_cast<float>(SCREEN_WIDTH) * 0.5f;
	const float centerY = static_cast<float>(SCREEN_HEIGHT) * 0.5f;
	constexpr float Gap = 4.0f;
	constexpr float Radius = 13.0f;

	std::vector<VERTEX_3D> screenVertices;
	screenVertices.reserve(8);
	AddLine(
		screenVertices,
		{ centerX - Radius, centerY, 0.0f },
		{ centerX - Gap, centerY, 0.0f },
		crosshairColor);
	AddLine(
		screenVertices,
		{ centerX + Gap, centerY, 0.0f },
		{ centerX + Radius, centerY, 0.0f },
		crosshairColor);
	AddLine(
		screenVertices,
		{ centerX, centerY - Radius, 0.0f },
		{ centerX, centerY - Gap, 0.0f },
		crosshairColor);
	AddLine(
		screenVertices,
		{ centerX, centerY + Gap, 0.0f },
		{ centerX, centerY + Radius, 0.0f },
		crosshairColor);

	DrawVertices(screenVertices, true);
}

void GrappleVisual::DrawVertices(
	const std::vector<VERTEX_3D>& Vertices,
	bool ScreenSpace)
{
	if (Vertices.empty())
	{
		return;
	}
	if (Vertices.size() > MaxVertexCount)
	{
		throw std::runtime_error("Grapple visual vertex count exceeds its buffer");
	}

	D3D11_MAPPED_SUBRESOURCE mapped{};
	ThrowIfFailed(
		Renderer::GetDeviceContext()->Map(
			m_VertexBuffer,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mapped),
		"ID3D11DeviceContext::Map(grapple visual)");

	memcpy_s(
		mapped.pData,
		sizeof(VERTEX_3D) * MaxVertexCount,
		Vertices.data(),
		sizeof(VERTEX_3D) * Vertices.size());
	Renderer::GetDeviceContext()->Unmap(m_VertexBuffer, 0);

	Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
	Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);

	if (ScreenSpace)
	{
		Renderer::SetDepthEnable(false);
		Renderer::SetWorldViewProjection2D();
	}
	else
	{
		Renderer::SetDepthEnable(true);
		Renderer::SetWorldMatrix(XMMatrixIdentity());
	}

	MATERIAL material{};
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.TextureEnable = false;
	Renderer::SetMaterial(material);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(
		0,
		1,
		&m_VertexBuffer,
		&stride,
		&offset);
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(
		D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	Renderer::GetDeviceContext()->Draw(
		static_cast<UINT>(Vertices.size()),
		0);

	if (ScreenSpace)
	{
		Renderer::SetDepthEnable(true);
	}
}
