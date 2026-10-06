#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "bullet.h"
#include "enemy.h"
#include "explosion.h"
#include "score.h"


void Bullet::Init()
{
    m_Layer = 2;
    //m_Position = { -5.0f,0.0f,0.0f };

    m_Scale = { 1.0f, 1.0f, 1.0f };
    m_Rotation = { 0.0f, 0.0f, 0.0f };

    ModelRenderer* modelRenderer = AddComponent<ModelRenderer>(this);
    modelRenderer->Load("asset\\model\\bullet.obj");



    // シェーダー読込
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\unlitTextureVS.cso");

    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\unlitTexturePS.cso");
}

void Bullet::Uninit()
{
    SafeRelease(m_VertexLayout);
    SafeRelease(m_VertexShader);
    SafeRelease(m_PixelShader);

    GameObject::Uninit();
}
void Bullet::Update(float DeltaTime)
{
    m_Position += m_Velocity * DeltaTime * 10.0f;

   // Enemy* enemy = Manager::GetGameObject<Enemy>();
   
    //敵との衝突判定
    auto enemies = Manager::GetGameObjects<Enemy>();
    for (auto enemy : enemies)
    {
        if (enemy->IsDestroyed())
        {
            continue;
        }

        Vector3 direction = enemy->GetPosition() - m_Position;
        float length = direction.length();

        if (length < 1.0f)
        {
            //enemy->SetDestroy();

            direction.normalize();

            enemy->AddDamage(1);
            enemy->Shake(m_Velocity * 0.01f);

            SetDestroy();

            Manager::AddGameObject<Explosion>()->SetPosition(enemy->GetPosition());

            Score* score = Manager::GetGameObject<Score>();
            if (score != nullptr)
            {
                score->Add(1);
            }

            break;
        }
    }

    m_Lifetime -= DeltaTime;
    if(m_Lifetime <= 0.0f)
    {
        SetDestroy();
    }
        //if (GetAsyncKeyState(VK_RETURN)) {
    //    m_Velocity += Vector3(10.0f, 0.0f, 0.0f) * dt;
     
    //}
    GameObject::Update(DeltaTime);
}

void Bullet::Draw()
{
    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    GameObject::Draw();
}
