#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "camera.h"
#include "sky.h"


void Sky::Init()
{
    m_Layer = 1;

    // m_Position = { -5.0f,0.0f,0.0f };

    ModelRenderer* modelRenderer = AddComponent<ModelRenderer>(this);
    modelRenderer->Load("asset\\model\\sky.obj");



    // シェーダー読込
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\unlitTextureVS.cso");

    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\unlitTexturePS.cso");
    m_Scale = { 100.0f,100.0f,100.0f };
}

void Sky::Uninit()
{
    SafeRelease(m_VertexLayout);
    SafeRelease(m_VertexShader);
    SafeRelease(m_PixelShader);

    GameObject::Uninit();
}
void Sky::Update(float)
{
    //Camera* camera = Manager::AddGameObject<Camera>();
    //m_Position = camera->GetPosition();
    //GameObject::Update();

}

void Sky::Draw()
{
    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    // マトリクス設定
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);

    GameObject::Draw();
}
