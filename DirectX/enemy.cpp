#include "main.h"
#include "input.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "camera.h"
#include "enemy.h"
#include "explosion.h"

void Enemy::Init()
{
    m_Layer = 1;

    m_Life = 6; m_Flash = false;
   // m_Position = { -5.0f,0.0f,0.0f };

    m_ModelRenderer = AddComponent<ModelRenderer>(this);
    m_ModelRenderer->Load("asset\\model\\player.obj");
    m_ModelRenderer->SetFlash(true);


    // シェーダー読込
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\unlitTextureVS.cso");

    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\unlitTexturePS.cso");
}

void Enemy::Uninit()
{
    SafeRelease(m_VertexLayout);
    SafeRelease(m_VertexShader);
    SafeRelease(m_PixelShader);

    GameObject::Uninit();
}
void Enemy::Update(float DeltaTime)
{
    m_Position += m_Shake * cosf(m_ShakeTime * 100.0f);
    m_ShakeTime += DeltaTime;
    m_Shake *= 0.9f;
    m_ModelRenderer->SetFlash(m_Flash);

    if (m_ShakeTime > 0.02f)
    {
        m_Flash = false;
    }
    GameObject::Update(DeltaTime);
}

void Enemy::Draw()
{
    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    GameObject::Draw();
}

void Enemy::AddDamage(int Damage)
{
    if (IsDestroyed())
    {
        return;
    }

    m_Life -= Damage;

    m_Flash = true;

    if (m_Life <= 0)
    {
        SetDestroy();

        Explosion* explosion = Manager::AddGameObject<Explosion>();
        explosion->SetPosition(m_Position);
        explosion->SetScale({ 2.0f, 2.0f, 2.0f });

        Camera* camera = Manager::GetGameObject<Camera>();
        if (camera != nullptr)
        {
            camera->Shake({ 1.0f, 0.0f, 0.0f });
        }
    }
}
