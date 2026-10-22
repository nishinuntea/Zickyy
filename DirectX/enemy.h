#pragma once

#include "gameObject.h"

class Enemy :public GameObject
{
private:
    Vector3 m_Velocity{ 0.0f,0.0f,0.0f };//速度

    // ID3D11Buffer* m_VertexBuffer;

    ID3D11InputLayout* m_VertexLayout{};
    ID3D11VertexShader* m_VertexShader{};
    ID3D11PixelShader* m_PixelShader{};

   class  ModelRenderer* m_ModelRenderer{};

    Vector3 m_Shake{ 0.0f, 0.0f, 0.0f };
    float m_ShakeTime{};
    // ID3D11ShaderResourceView* m_Texture;

    int m_Life{};
    bool m_Flash{};

public:
    void Init()override;
    void Uninit()override;
    void Update(float DeltaTime)override;
    void Draw()override;

    void Shake(Vector3 Shake)
    {
        m_Shake = Shake;
        m_ShakeTime = 0.0f;
    
    }

    void AddDamage(int Damage);

};
