#pragma once
#include "gameObject.h"
#include "runnerLogic.h"
#include "runnerCamera.h"
#include <string>

class Audio;
class Runner final : public GameObject
{
    RelicRun::Run m_Run;
    RelicRun::Pose m_Pose;
    RelicRun::PathPoint m_PathOrigin;
    RelicRun::WallCameraFeedback m_WallFeedback;
    std::array<Audio*, 7> m_Sounds{};
    bool m_Muted{}, m_WorldPass{}, m_Demo{};
    int m_ForkOverride{};
    float m_RenderOffset{}, m_Impact{}, m_ActionFlash{}, m_LandFlash{}, m_TurnFlash{};
    ID3D11Buffer* m_Buffer{};
    ID3D11InputLayout* m_Layout{};
    ID3D11VertexShader* m_VS{};
    ID3D11PixelShader* m_PS{};
    ID3D11DepthStencilState* m_HudDepth{};
    std::vector<VERTEX_3D> m_Vertices;
    float m_Time{}, m_TitleTimer{}, m_FrameMs = 16.67f;
    void Box(float x, float y, float z, float w, float h, float d, XMFLOAT4 color);
    void Rect(float x, float y, float w, float h, XMFLOAT4 color);
    void Text(const std::string& text, float x, float y, float size, XMFLOAT4 color);
    void Flush();
    void DrawCourse();
    void DrawHud();
    XMFLOAT3 OnPath(float x, float y, float z) const;
    void PlaySound(size_t sound);
public:
    void Init() override { Init(false); }
    void Init(bool start);
    void Uninit() override;
    void Update(float dt) override;
    void Draw() override;
};
