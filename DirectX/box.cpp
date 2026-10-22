#include "main.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "box.h"

void Box::Init()
{
	m_Layer = 1;

	ModelRenderer* modelRenderer = AddComponent<ModelRenderer>(this);
	modelRenderer->Load("asset\\model\\box.obj");

	Renderer::CreateVertexShader(
		&m_VertexShader,
		&m_VertexLayout,
		"shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(
		&m_PixelShader,
		"shader\\unlitTexturePS.cso");
}

void Box::Uninit()
{
	SafeRelease(m_VertexLayout);
	SafeRelease(m_VertexShader);
	SafeRelease(m_PixelShader);

	GameObject::Uninit();
}

void Box::Update(float DeltaTime)
{
	GameObject::Update(DeltaTime);
}

void Box::Draw()
{
	Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
	Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);

	GameObject::Draw();
}
