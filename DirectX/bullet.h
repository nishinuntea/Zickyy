#pragma once

#include "gameObject.h"

class Bullet :public GameObject
{
private:
    Vector3 m_Velocity{ 0.0f,0.0f,0.0f };//速度

    // ID3D11Buffer* m_VertexBuffer;

    ID3D11InputLayout* m_VertexLayout{};
    ID3D11VertexShader* m_VertexShader{};
    ID3D11PixelShader* m_PixelShader{};

    float m_Lifetime{ 2.0f };
    // ID3D11ShaderResourceView* m_Texture;

    class ModelRenderer* m_ModelRenderer{};//前方宣言

public:
    void Init()override;
    void Uninit()override;
    void Update(float DeltaTime)override;
    void Draw()override;

    void SetVelocity(const Vector3& Velocity) { m_Velocity = Velocity; }
};
