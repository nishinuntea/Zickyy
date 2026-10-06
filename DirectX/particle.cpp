#include "main.h"
#include "renderer.h"
#include "manager.h"
#include "camera.h"
#include "particle.h"

void Particle::Init()
{
    m_Layer = 2;

    VERTEX_3D vertex[4];

    vertex[0].Position = XMFLOAT3(-0.5f, 0.5f, 0.0f);
    vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

    vertex[1].Position = XMFLOAT3(0.5f, 0.5f, 0.0f);
    vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

    vertex[2].Position = XMFLOAT3(-0.5f, -0.5f, 0.0f);
    vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

    vertex[3].Position = XMFLOAT3(0.5f, -0.5f, 0.0f);
    vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

    // 頂点バッファ生成
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(VERTEX_3D) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;

    ThrowIfFailed(Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer),
        "ID3D11Device::CreateBuffer(particle)");

    // テクスチャ読込
    Renderer::CreateTextureFromFile(&m_Texture, L"asset\\texture\\particle.png");

    // シェーダー読込
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\unlitTextureVS.cso");

    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\unlitTexturePS.cso");

    //パーティクル初期化
    for (int i = 0; i < PARTICLE_MAX; i++)
    {
        m_Particle[i].Enable = false;
    }

}

void Particle::Uninit()
{
    SafeRelease(m_VertexBuffer);
    SafeRelease(m_Texture);
    SafeRelease(m_VertexLayout);
    SafeRelease(m_VertexShader);
    SafeRelease(m_PixelShader);
}
void Particle::Update(float DeltaTime)
{
    //パーティクル発射
    for (int i = 0; i < PARTICLE_MAX; i++)
    {
        if (m_Particle[i].Enable == false)
        {
            m_Particle[i].Enable = true;
            m_Particle[i].Life = 60;
            m_Particle[i].Position = m_Position;
            m_Particle[i].Velocity.x = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f;
            m_Particle[i].Velocity.y = ((float)rand() / RAND_MAX) * 10.0f;
            m_Particle[i].Velocity.z = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f;
            break;
        }
    }

    //パーティクル更新
    
    Vector3 gravity{ 0.0f,-9.8f,0.0f };//重力加速度

    for (int i = 0; i < PARTICLE_MAX; i++)
    {
        if (m_Particle[i].Enable == true)
        {
            m_Particle[i].Velocity += gravity * DeltaTime;
            m_Particle[i].Position += m_Particle[i].Velocity * DeltaTime;

            m_Particle[i].Life--;
            if (m_Particle[i].Life < 0)
            {
                m_Particle[i].Enable = false;
            }
        }
    }
}

void Particle::Draw()
{
    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);


    // テクスチャ設定
    if (m_Texture) {
        Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);
    }

    // 頂点バッファ設定
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);


    // プリミティブトポロジ設定
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


    //ビルボード用マトリクス
    Camera* camera = Manager::GetGameObject<Camera>();
    XMMATRIX view = camera->GetViewMatrix();
    XMMATRIX invView = XMMatrixInverse(NULL, view);//逆行列
    invView.r[3].m128_f32[0] = 0.0f;
    invView.r[3].m128_f32[1] = 0.0f;
    invView.r[3].m128_f32[2] = 0.0f;

    // マテリアル設定
    MATERIAL material{};
    material.Diffuse = { 0.2f, 0.2f, 1.0f, 1.0f };
    material.TextureEnable = true;
    Renderer::SetMaterial(material);

    // パーティクル用のステート設定
    Renderer::SetDepthEnable(false);
    // 加算合成
    Renderer::SetAddBlendEnable(true);

    for (int i = 0; i < PARTICLE_MAX; i++)
    {
        if (m_Particle[i].Enable == true)
        {
            // マトリクス設定
            XMMATRIX world, scale, trans;
            scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
            trans = XMMatrixTranslation(m_Particle[i].Position.x,
                m_Particle[i].Position.y,
                m_Particle[i].Position.z);

            world = scale * invView * trans;
            Renderer::SetWorldMatrix(world);

            // ポリゴン描画
            Renderer::GetDeviceContext()->Draw(4, 0);
        }
    }
    //for (int i = 0; i < PARTICLE_MAX; i++)
    //{
    //    if (m_Particle[i].Enable == true)
    //    {

    //        // マトリクス設定
    //        XMMATRIX world, scale, rot, trans;
    //        scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    //        //rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    //        trans = XMMatrixTranslation(m_Particle[i].Position.x, m_Particle[i].Position.y, m_Particle[i].Position.z);
    //        world = scale * invView * trans;


    //        Renderer::SetWorldMatrix(world);

    //        // ポリゴン描画
    //        Renderer::GetDeviceContext()->Draw(4, 0);
    //    }
    //}

    // 加算合成
    Renderer::SetAddBlendEnable(false);
    Renderer::SetDepthEnable(true);

}
