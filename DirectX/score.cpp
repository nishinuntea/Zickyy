#include "main.h"
#include "renderer.h"
#include "score.h"

void Score::Init()
{
    m_Layer = 3;

    VERTEX_3D vertex[4];

    vertex[0].Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
    vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

    vertex[1].Position = XMFLOAT3(50.0f, 0.0, 0.0f);
    vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
    vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

    vertex[2].Position = XMFLOAT3(0.0f, 50.0f, 0.0f);
    vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
    vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

    vertex[3].Position = XMFLOAT3(50.0f,50.0f, 0.0f);
    vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
    vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

    // 頂点バッファ生成
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(VERTEX_3D) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;

    ThrowIfFailed(Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer),
        "ID3D11Device::CreateBuffer(score)");

    // シェーダー読込
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\unlitTextureVS.cso");

    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\unlitTexturePS.cso");

    // テクスチャ読込
    Renderer::CreateTextureFromFile(&m_Texture, L"asset\\texture\\number.png");

    m_Value = 0;
}

void Score::Uninit()
{
    SafeRelease(m_Texture);
    SafeRelease(m_VertexBuffer);
    SafeRelease(m_VertexLayout);
    SafeRelease(m_VertexShader);
    SafeRelease(m_PixelShader);
}
void Score::Update(float)
{

}

void Score::Draw()
{
    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    // マトリクス設定
    Renderer::SetWorldViewProjection2D();

    // マトリクス設定
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);

    // マテリアル設定
    MATERIAL material{};
    material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
    material.TextureEnable = true;
    Renderer::SetMaterial(material);

    // テクスチャ設定
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);

    // 頂点バッファ設定
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);

    // プリミティブトポロジ設定
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
   
    int number = m_Value;
    
    for (int i=0;i<4;i++)
    {
        // 頂点データ変更
        int num = number % 10;
        number /= 10;

        D3D11_MAPPED_SUBRESOURCE msr;
        ThrowIfFailed(Renderer::GetDeviceContext()->Map(
            m_VertexBuffer,
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &msr),
            "ID3D11DeviceContext::Map(score)");

        VERTEX_3D* vertex = (VERTEX_3D*)msr.pData;

        float x = -30.0f * i;

        float tx = 1.0f / 5.0f * (num % 5);
        float ty = 1.0f / 5.0f * (num / 5);
        float tw = 1.0f / 5.0f;
        float th = 1.0f / 5.0f;
        vertex[0].Position = XMFLOAT3(x, 0.0f, 0.0f);
        vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[0].TexCoord = XMFLOAT2(tx, ty);

        vertex[1].Position = XMFLOAT3(x+50.0f, 0.0f, 0.0f);
        vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[1].TexCoord = XMFLOAT2(tx + tw, ty);

        vertex[2].Position = XMFLOAT3(x+0.0f, 50.0f, 0.0f);
        vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[2].TexCoord = XMFLOAT2(tx, ty + th);

        vertex[3].Position = XMFLOAT3(x+50.0f, 50.0f, 0.0f);
        vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[3].TexCoord = XMFLOAT2(tx + tw, ty + th);

        Renderer::GetDeviceContext()->Unmap(m_VertexBuffer, 0);

        // ポリゴン描画
        Renderer::GetDeviceContext()->Draw(4, 0);

    }

}
