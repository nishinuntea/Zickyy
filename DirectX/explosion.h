#pragma once

#include "gameObject.h"

class Explosion :public GameObject
{
private:
    ID3D11Buffer* m_VertexBuffer{};

    ID3D11InputLayout* m_VertexLayout{};
    ID3D11VertexShader* m_VertexShader{};
    ID3D11PixelShader* m_PixelShader{};

    ID3D11ShaderResourceView* m_Texture{};

    int m_Frame{};
public:
    void Init()override;
    void Uninit()override;
    void Update(float DeltaTime)override;
    void Draw()override;
};
