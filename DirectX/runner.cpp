#include "main.h"
#include "renderer.h"
#include "runner.h"
#include "input.h"
#include "manager.h"
#include "audio.h"
#include <chrono>

namespace
{
constexpr unsigned int VertexCapacity = 120000;
const XMFLOAT4 Ivory{0.94f, 0.91f, 0.77f, 1.0f};
const XMFLOAT4 Gold{1.0f, 0.72f, 0.22f, 1.0f};
const XMFLOAT4 Teal{0.18f, 0.79f, 0.69f, 1.0f};
// Original 5x7 bitmap lettering. No external font or UI texture dependency.
const char* Glyph(char c)
{
    static const char* letters[] = {
        "01110100011000111111100011000110001", "11110100011000111110100011000111110",
        "01111100001000010000100001000001111", "11110100011000110001100011000111110",
        "11111100001000011110100001000011111", "11111100001000011110100001000010000",
        "01111100001000010111100011000101111", "10001100011000111111100011000110001",
        "11111001000010000100001000010011111", "00111000100001000010000101001001100",
        "10001100101010011000101001001010001", "10000100001000010000100001000011111",
        "10001110111010110101100011000110001", "10001110011010110011100011000110001",
        "01110100011000110001100011000101110", "11110100011000111110100001000010000",
        "01110100011000110001101011001001101", "11110100011000111110101001001010001",
        "01111100001000001110000010000111110", "11111001000010000100001000010000100",
        "10001100011000110001100011000101110", "10001100011000110001100010101000100",
        "10001100011000110101101011101110001", "10001100010101000100010101000110001",
        "10001100010101000100001000010000100", "11111000010001000100010001000011111"
    };
    static const char* digits[] = {
        "01110100011001110101110011000101110", "00100011000010000100001000010001110",
        "01110100010000100010001000100011111", "11110000010000101110000010000111110",
        "00010001100101010010111110001000010", "11111100001000011110000010000111110",
        "01110100001000011110100011000101110", "11111000010001000100010000100001000",
        "01110100011000101110100011000101110", "01110100011000101111000010000101110"
    };
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
    if (c >= '0' && c <= '9') return digits[c - '0'];
    if (c == '-') return "00000000000000011111000000000000000";
    if (c == '.') return "00000000000000000000000000011000110";
    if (c == ':') return "00000001000010000000001000010000000";
    if (c == '/') return "00001000100001000100010000100010000";
    return "00000000000000000000000000000000000";
}
}

void Runner::Init(bool start)
{
    m_Demo = wcsstr(GetCommandLineW(), L"--runner-demo") != nullptr;
    m_Run.Reset(start || m_Demo);
    Input::SetMouseLookEnabled(false);
    m_Vertices.reserve(VertexCapacity);
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(VERTEX_3D) * VertexCapacity;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ThrowIfFailed(Renderer::GetDevice()->CreateBuffer(&desc, nullptr, &m_Buffer), "Create runner batch");
    Renderer::CreateVertexShader(&m_VS, &m_Layout, "shader\\unlitTextureVS.cso");
    Renderer::CreatePixelShader(&m_PS, "shader\\unlitTexturePS.cso");
    D3D11_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = FALSE;
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D11_COMPARISON_ALWAYS;
    ThrowIfFailed(Renderer::GetDevice()->CreateDepthStencilState(&depth, &m_HudDepth), "Create runner HUD state");
    const float tones[7][3] = {{980,1600,0.13f},{280,720,0.18f},{450,90,0.22f},
        {100,45,0.13f},{140,35,0.48f},{700,1100,0.25f},{500,850,0.20f}};
    for (size_t i = 0; i < m_Sounds.size(); ++i)
    {
        m_Sounds[i] = AddComponent<Audio>(this);
        m_Sounds[i]->LoadTone(tones[i][0], tones[i][1], tones[i][2], i == 4 ? 0.25f : 0.12f);
    }
}

void Runner::Uninit()
{
    GameObject::Uninit();
    m_Sounds = {};
    SafeRelease(m_Buffer); SafeRelease(m_Layout);
    SafeRelease(m_VS); SafeRelease(m_PS); SafeRelease(m_HudDepth);
}

void Runner::Update(float dt)
{
    if (Input::GetKeyTrigger('M')) m_Muted = !m_Muted;
    if (Input::GetKeyTrigger(VK_F3))
    {
        m_Demo = !m_Demo;
        m_Run.Reset(m_Demo);
        m_WallFeedback.Reset();
        m_Impact = m_ActionFlash = m_LandFlash = m_TurnFlash = 0;
    }
    if (!m_Demo && m_Run.phase == RelicRun::Phase::Playing && GetForegroundWindow() != GetWindow())
        m_Run.phase = RelicRun::Phase::Paused;
    const bool start = Input::GetKeyTrigger(VK_RETURN);
    if ((m_Run.phase == RelicRun::Phase::Ready || m_Run.phase == RelicRun::Phase::Dead) && start)
    { m_Run.Reset(); m_WallFeedback.Reset(); m_Impact = m_ActionFlash = m_LandFlash = m_TurnFlash = 0; }
    else if (m_Run.phase == RelicRun::Phase::Paused && start)
        m_Run.TogglePause();
    if (Input::GetKeyTrigger('R') && m_Run.phase != RelicRun::Phase::Ready)
    { m_Run.Reset(); m_WallFeedback.Reset(); m_Impact = m_ActionFlash = m_LandFlash = m_TurnFlash = 0; }
    if (Input::GetKeyTrigger('P')) m_Run.TogglePause();
    RelicRun::Input input;
    input.laneDelta = static_cast<int>(Input::GetKeyTrigger('D') || Input::GetKeyTrigger(VK_RIGHT)) -
        static_cast<int>(Input::GetKeyTrigger('A') || Input::GetKeyTrigger(VK_LEFT));
    input.jump = Input::GetKeyTrigger('W') || Input::GetKeyTrigger(VK_UP) || Input::GetKeyTrigger(VK_SPACE);
    input.slide = Input::GetKeyTrigger('S') || Input::GetKeyTrigger(VK_DOWN);
    if (m_Demo)
    {
        const auto row = std::find_if(m_Run.rows.begin(),m_Run.rows.end(),
            [](const RelicRun::Row& r){return r.z > -1.8f;});
        input = {};
        if (row != m_Run.rows.end()) input.laneDelta = row->safeLane-1-m_Run.lane;
        const auto* junction = m_Run.NextJunction();
        if (junction && junction->z <= 32) input.laneDelta = junction->type == RelicRun::Turn::Left ? -1 : 1;
    }
    m_Run.Step(dt, input);
    m_WallFeedback.Update(dt,m_Run.events.wallHitDirection,m_Run.phase == RelicRun::Phase::Paused);
    if (m_Run.phase != RelicRun::Phase::Paused)
    {
        m_Impact = std::max(0.0f,m_Impact-dt);
        m_ActionFlash = std::max(0.0f,m_ActionFlash-dt);
        m_LandFlash = std::max(0.0f,m_LandFlash-dt);
        m_TurnFlash = std::max(0.0f,m_TurnFlash-dt);
    }
    if (m_Run.events.coin) PlaySound(0);
    if (m_Run.events.jump) { PlaySound(1); m_ActionFlash = 0.4f; }
    if (m_Run.events.slide) { PlaySound(2); m_ActionFlash = 0.4f; }
    if (m_Run.events.land) { PlaySound(3); m_LandFlash = 0.2f; }
    if (m_Run.events.crash) { PlaySound(4); m_Impact = 0.7f; }
    if (m_Run.events.turn) { PlaySound(5); m_TurnFlash = 1.1f; }
    if (m_Run.events.warning) PlaySound(6);
    if (m_Run.phase != RelicRun::Phase::Paused && m_Run.phase != RelicRun::Phase::Dead) m_Time += dt;
    m_Position = {m_Run.x, m_Run.y, 0.0f};
    m_TitleTimer += dt;
    if (m_TitleTimer > 0.25f)
    {
        m_TitleTimer = 0.0f;
        wchar_t title[160];
        swprintf_s(title, L"RELIC RUN | %d m | Coins %d | Score %d | %.1f ms | P Pause | R Retry",
            static_cast<int>(m_Run.distance), m_Run.coinCount, m_Run.Score(), m_FrameMs);
        SetWindowTextW(GetWindow(), title);
    }
}

void Runner::PlaySound(size_t sound)
{
    if (!m_Muted) m_Sounds.at(sound)->Play();
}

XMFLOAT3 Runner::OnPath(float x, float y, float z) const
{
    const auto p = RelicRun::SamplePath(z,m_Run.junctions,m_RenderOffset,m_ForkOverride);
    const float dx = p.x + x*std::cos(p.angle) - m_PathOrigin.x;
    const float dz = p.z - x*std::sin(p.angle) - m_PathOrigin.z;
    const float c = std::cos(m_PathOrigin.angle), s = std::sin(m_PathOrigin.angle);
    return {dx*c-dz*s,y,dx*s+dz*c};
}

void Runner::Box(float x, float y, float z, float w, float h, float d, XMFLOAT4 color)
{
    XMFLOAT3 p[8] = {
        {x-w/2,y-h/2,z-d/2},{x-w/2,y+h/2,z-d/2},{x+w/2,y+h/2,z-d/2},{x+w/2,y-h/2,z-d/2},
        {x-w/2,y-h/2,z+d/2},{x-w/2,y+h/2,z+d/2},{x+w/2,y+h/2,z+d/2},{x+w/2,y-h/2,z+d/2}};
    if (m_WorldPass)
    {
        // Transform shared slab edges onto the curve, avoiding wedge-shaped gaps.
        for (auto& vertex : p)
            vertex = OnPath(vertex.x,vertex.y,vertex.z);
    }
    const int faces[6][6] = {{0,1,2,0,2,3},{7,6,5,7,5,4},{4,5,1,4,1,0},
        {3,2,6,3,6,7},{1,5,6,1,6,2},{4,0,3,4,3,7}};
    const float shade[6] = {0.78f,0.65f,0.68f,0.88f,1.0f,0.55f};
    for (int f = 0; f < 6; ++f)
        for (int i : faces[f]) m_Vertices.push_back({p[i], {0,1,0},
            {color.x*shade[f],color.y*shade[f],color.z*shade[f],color.w},{0,0}});
}

void Runner::Rect(float x, float y, float w, float h, XMFLOAT4 color)
{
    const XMFLOAT3 p[] = {{x,y,0},{x+w,y,0},{x,y+h,0},{x,y+h,0},{x+w,y,0},{x+w,y+h,0}};
    for (auto position : p) m_Vertices.push_back({position, {0,0,-1},color,{0,0}});
}

void Runner::Text(const std::string& text, float x, float y, float size, XMFLOAT4 color)
{
    for (char c : text)
    {
        const char* glyph = Glyph(c);
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (glyph[row*5+col] == '1') Rect(x+col*size,y+row*size,size,size,color);
        x += size * 6.0f;
    }
}

void Runner::Flush()
{
    if (m_Vertices.empty()) return;
    if (m_Vertices.size() > VertexCapacity) throw std::runtime_error("Runner vertex batch overflow");
    auto* context = Renderer::GetDeviceContext();
    D3D11_MAPPED_SUBRESOURCE mapped{};
    ThrowIfFailed(context->Map(m_Buffer,0,D3D11_MAP_WRITE_DISCARD,0,&mapped), "Map runner batch");
    memcpy(mapped.pData, m_Vertices.data(), m_Vertices.size()*sizeof(VERTEX_3D));
    context->Unmap(m_Buffer,0);
    context->IASetInputLayout(m_Layout);
    context->VSSetShader(m_VS,nullptr,0);
    context->PSSetShader(m_PS,nullptr,0);
    UINT stride = sizeof(VERTEX_3D), offset = 0;
    context->IASetVertexBuffers(0,1,&m_Buffer,&stride,&offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    MATERIAL material{};
    material.Diffuse = {1,1,1,1};
    Renderer::SetMaterial(material);
    Renderer::SetWorldMatrix(XMMatrixIdentity());
    context->Draw(static_cast<UINT>(m_Vertices.size()),0);
    m_Vertices.clear();
}

void Runner::DrawCourse()
{
    const float scroll = std::fmod(m_Pose.distance, 12.0f);
    // Small road slabs follow the curved centerline; holes really remove floor.
    for (int tile = 0; tile < 110; ++tile)
    {
        const float start = tile*2.0f-std::fmod(m_Pose.distance,2.0f)-16.0f;
        for (int lane = -1; lane <= 1; ++lane)
        {
            std::vector<std::pair<float,float>> parts{{start,start+2.0f}};
            for (const auto& row : m_Run.rows)
            {
                if (row.obstacles[lane+1] != RelicRun::Obstacle::Gap) continue;
                const float gapStart = row.z+m_RenderOffset-1.8f, gapEnd = gapStart+3.6f;
                std::vector<std::pair<float,float>> clipped;
                for (const auto& part : parts)
                {
                    if (part.second <= gapStart || part.first >= gapEnd) clipped.push_back(part);
                    else
                    {
                        if (part.first < gapStart) clipped.push_back({part.first,gapStart});
                        if (part.second > gapEnd) clipped.push_back({gapEnd,part.second});
                    }
                }
                parts = std::move(clipped);
            }
            for (const auto& part : parts)
                Box(lane*3.0f,-0.3f,(part.first+part.second)*0.5f,3.0f,0.6f,part.second-part.first+0.01f,
                    (tile+static_cast<int>(m_Pose.distance/2))%2 ?
                    XMFLOAT4{0.51f,0.51f,0.39f,1} : XMFLOAT4{0.46f,0.47f,0.35f,1});
        }
        for (float side : {-1.0f,1.0f})
        {
            Box(side*4.9f,0.22f,start+1,0.55f,0.65f,2.08f,{0.66f,0.60f,0.40f,1});
            Box(side*1.5f,0.025f,start+1,0.045f,0.02f,2.05f,Gold);
        }
    }
    for (int i = 0; i < 19; ++i)
    {
        const float z = i*12.0f - scroll - 12.0f;
        for (float side : {-1.0f,1.0f})
        {
            Box(side*6.5f,2.5f,z,1.6f,5.0f,1.6f,{0.34f,0.40f,0.31f,1});
            Box(side*6.5f,5.1f,z,2.3f,0.55f,2.3f,{0.54f,0.53f,0.37f,1});
            Box(side*6.5f,0.35f,z,2.3f,0.7f,2.3f,{0.54f,0.53f,0.37f,1});
            Box(side*5.65f,3.4f,z,0.25f,0.9f,0.4f,Gold);
            Box(side*10.5f,2.0f,z+4,4.0f,4.0f,4.0f,{0.10f,0.28f,0.23f,1});
            Box(side*10.5f,5.0f,z+4,5.8f,3.0f,5.8f,{0.13f,0.35f,0.27f,1});
        }
    }
    for (const auto& junction : m_Run.junctions)
    {
        const float z = junction.z+m_RenderOffset;
        if (z < -25 || z > 180) continue;
        const XMFLOAT4 signColor = junction.choice != 0 ? Teal : Gold;
        // Remove a passed sign before it can sit between camera and player.
        if (z > 16)
        {
        Box(0,4.4f,z-12,8.7f,1.5f,0.35f,{0.06f,0.20f,0.19f,1});
        for (int direction : {-1,1})
        {
            if ((junction.type == RelicRun::Turn::Left && direction > 0) ||
                (junction.type == RelicRun::Turn::Right && direction < 0)) continue;
            const float cx = junction.type == RelicRun::Turn::Fork ? direction*2.0f : 0.0f;
            Box(cx,4.4f,z-12.22f,2.0f,0.16f,0.07f,signColor);
            for (int dot = 0; dot < 4; ++dot)
            {
                Box(cx+direction*(1.0f-dot*0.18f),4.4f+dot*0.15f,z-12.22f,0.2f,0.18f,0.07f,signColor);
                Box(cx+direction*(1.0f-dot*0.18f),4.4f-dot*0.15f,z-12.22f,0.2f,0.18f,0.07f,signColor);
            }
        }
        }
        if (junction.type == RelicRun::Turn::Fork && !junction.resolved)
        {
            m_ForkOverride = junction.choice > 0 ? -1 : 1;
            for (int tile = 0; tile < 35; ++tile)
            {
                const float branchZ = z + tile*2.0f + 1.0f;
                Box(0,-0.3f,branchZ,9,0.6f,2.1f,{0.38f,0.44f,0.36f,1});
                Box(-4.9f,0.22f,branchZ,0.55f,0.65f,2.1f,Gold);
                Box(4.9f,0.22f,branchZ,0.55f,0.65f,2.1f,Gold);
            }
            m_ForkOverride = 0;
        }
    }
    for (const auto& row : m_Run.rows)
    {
        for (int i = 0; i < 3; ++i)
        {
            const float x = (i-1)*3.0f;
            const float z = row.z + m_RenderOffset;
            switch (row.obstacles[i])
            {
            case RelicRun::Obstacle::Wall:
                Box(x,1.65f,z,2.05f,3.3f,1.5f,{0.65f,0.32f,0.23f,1});
                Box(x,3.35f,z,2.35f,0.25f,1.8f,Gold);
                Box(x,1.65f,z-0.78f,0.25f,2.2f,0.05f,Gold);
                break;
            case RelicRun::Obstacle::Hurdle:
                Box(x,0.5f,z,2.1f,1.0f,1.3f,{0.93f,0.56f,0.20f,1});
                Box(x,1.03f,z,2.2f,0.12f,1.4f,Gold);
                for (int mark = -1; mark <= 1; ++mark)
                    Box(x+mark*0.55f,0.5f,z-0.67f,0.14f,0.75f,0.03f,Ivory);
                break;
            case RelicRun::Obstacle::Beam:
                Box(x,2.05f,z,2.2f,2.0f,1.3f,{0.14f,0.58f,0.53f,1});
                Box(x,1.10f,z-0.7f,2.2f,0.15f,0.12f,Teal);
                Box(x-1.0f,0.5f,z,0.12f,1.0f,1.1f,Teal);
                Box(x+1.0f,0.5f,z,0.12f,1.0f,1.1f,Teal);
                break;
            case RelicRun::Obstacle::Gap:
                Box(x,-3.5f,z,2.98f,0.1f,3.6f,{0.01f,0.025f,0.03f,1});
                Box(x,0.025f,z-1.88f,2.98f,0.05f,0.16f,Gold);
                Box(x,0.025f,z+1.88f,2.98f,0.05f,0.16f,Gold);
                break;
            case RelicRun::Obstacle::None: break;
            }
        }
    }
    for (const auto& coin : m_Run.coins)
    {
        const float z = coin.z + m_RenderOffset;
        const float width = 0.18f + 0.38f * std::abs(std::cos(m_Time*4+coin.z*0.1f));
        Box(coin.x,1.0f+std::sin(m_Time*3+z)*0.08f,z,width,0.63f,0.18f,Gold);
        Box(coin.x,1.0f,z-0.10f,width*0.45f,0.31f,0.03f,Ivory);
    }
    // Simple contact shadow, placed above the road to avoid depth fighting.
    if (m_Run.death != RelicRun::Death::Fall)
        Box(m_Pose.x,0.04f,0,0.85f,0.025f,0.65f,{0.18f,0.22f,0.18f,1});
    Flush();
}

void Runner::DrawHud()
{
    Rect(24,22,1232,88,{0.025f,0.10f,0.10f,0.92f});
    Rect(24,22,4,88,Gold);
    Text("RELIC RUN",48,38,3,Gold);
    Text("THE EMERALD CAUSEWAY",48,72,1.6f,Ivory);
    Text("DISTANCE",405,36,1.8f,Teal);
    Text(std::to_string(static_cast<int>(m_Run.distance))+" M",405,62,3,Ivory);
    Text("COINS",680,36,1.8f,Teal);
    Text(std::to_string(m_Run.coinCount),680,62,3,m_Run.collectFlash>0 ? Ivory : Gold);
    Text("SCORE",920,36,1.8f,Teal);
    Text(std::to_string(m_Run.Score()),920,62,3,Ivory);
    Rect(24,657,1232,43,{0.025f,0.10f,0.10f,0.90f});
    Text("A/D LANE/TURN   W/SPACE JUMP   S SLIDE   P PAUSE   R RETRY   M SOUND",48,672,1.9f,Ivory);
    Text(m_Muted ? "SOUND OFF" : "SOUND ON",1100,120,1.5f,m_Muted?Ivory:Teal);
    if (m_Demo) Text("AUTO DEMO",48,145,2,Gold);
    if (m_Run.collectFlash > 0)
        Text("+25",725,132-m_Run.collectFlash*35,3,Gold);
    if (m_Impact > 0)
    {
        const XMFLOAT4 impact{0.9f,0.16f,0.08f,m_Impact*0.65f};
        Rect(0,0,1280,12,impact); Rect(0,708,1280,12,impact);
        Rect(0,0,12,720,impact); Rect(1268,0,12,720,impact);
    }
    if (m_Run.phase == RelicRun::Phase::Playing)
    {
        Rect(24,117,220,5,{0.1f,0.2f,0.18f,1});
        Rect(24,117,220*(m_Run.speed/RelicRun::Run::MaximumSpeed),5,Teal);
        char speedLabel[48];
        sprintf_s(speedLabel,"SPEED %.1f / %.0f",m_Run.speed,RelicRun::Run::MaximumSpeed);
        Text(speedLabel,260,117,1.8f,Teal);
        const auto* junction = m_Run.NextJunction();
        if (junction != nullptr && junction->z < 75)
        {
            const bool active = junction->z <= 32;
            const std::string direction = junction->type == RelicRun::Turn::Left ? "LEFT - A" :
                junction->type == RelicRun::Turn::Right ? "RIGHT - D" : "CHOOSE A OR D";
            Rect(275,166,730,95,{0.025f,0.10f,0.10f,0.95f});
            Text((active ? "TURN NOW: " : "TURN AHEAD: ")+direction,299,181,2.5f,Gold);
            Text("IN "+std::to_string(static_cast<int>(std::max(0.0f,junction->z)))+" M",299,218,2,Ivory);
            if (junction->choice != 0)
            {
                const bool valid = junction->type == RelicRun::Turn::Fork ||
                    (junction->type == RelicRun::Turn::Left ? junction->choice < 0 : junction->choice > 0);
                Text(valid ? (junction->choice < 0 ? "LEFT READY" : "RIGHT READY") : "WRONG DIRECTION",
                    625,218,2,valid?Teal:Gold);
            }
        }
        else if (m_TurnFlash > 0) Text("TURN CLEAR",505,190,3,Teal);
        const bool jumping = m_Run.y > 0.01f;
        Rect(30,530,215,74,{0.025f,0.10f,0.10f,0.92f});
        Text(jumping?"JUMP":m_Run.Sliding()?"SLIDE":"READY",45,547,3,m_ActionFlash>0?Gold:Teal);
        if (m_Run.Sliding()) Rect(45,581,180*m_Run.slideTime/0.8f,6,Teal);
        else if (jumping) Rect(45,581,180*std::min(1.0f,m_Run.y/2.15f),6,Gold);
        if (m_Run.distance < 127 && (junction == nullptr || junction->z >= 75))
        {
            const std::string hint = m_Run.distance < 27 ? "COLLECT GOLD - CHOOSE YOUR LANE" :
                m_Run.distance < 51 ? "AMBER BARRIER - JUMP OR DODGE" :
                m_Run.distance < 76 ? "TEAL BEAM - SLIDE OR DODGE" :
                m_Run.distance < 100 ? "RED WALL - CHANGE LANE" : "DARK GAP - JUMP OR CHANGE LANE";
            Rect(305,570,670,48,{0.025f,0.10f,0.10f,0.85f});
            Text(hint,325,586,2,Gold);
        }
    }
    else if (m_Run.phase != RelicRun::Phase::Dead || m_Impact < 0.15f)
    {
        Rect(0,130,1280,500,{0.015f,0.065f,0.06f,0.70f});
        Rect(260,178,760,390,{0.03f,0.12f,0.12f,0.96f});
        Rect(260,178,760,4,Gold);
        const bool ready = m_Run.phase == RelicRun::Phase::Ready;
        const bool dead = m_Run.phase == RelicRun::Phase::Dead;
        const std::string heading = ready ? "RELIC RUN" : dead ? "RUN ENDED" : "PAUSED";
        Text(heading,640-heading.size()*21.0f,220,7,Gold);
        const std::string failure = m_Run.death == RelicRun::Death::Fall ? "FELL INTO GAP - JUMP OR DODGE" :
            m_Run.death == RelicRun::Death::MissedTurn ? "MISSED TURN - CHOOSE A/D IN TIME" : "HIT OBSTACLE - JUMP SLIDE OR DODGE";
        Text(ready ? "CHASE THE GOLD. OUTRUN THE RUINS." : dead ? failure :
            "TAKE A BREATH. THE RUINS CAN WAIT.",320,298,2,Ivory);
        Text("JUMP GAPS   WATCH ARROWS   CHOOSE A/D",305,350,2,Teal);
        Text("BEST THIS SESSION  " + std::to_string(m_Run.bestScore),340,400,2,Ivory);
        if (ready) Text("F3 - AUTO TOUR",490,430,1.6f,Teal);
        Rect(390,455,500,66,Gold);
        Text(ready ? "ENTER - START RUN" : dead ? "ENTER - TRY AGAIN" : "ENTER - RESUME",424,478,3,
            {0.03f,0.12f,0.12f,1});
    }
    Flush();
}

void Runner::Draw()
{
    using Clock = std::chrono::steady_clock;
    static auto previous = Clock::now();
    const auto now = Clock::now();
    const float frameMs = std::chrono::duration<float,std::milli>(now-previous).count();
    previous = now;
    if (frameMs > 0 && frameMs < 500) m_FrameMs += (frameMs-m_FrameMs)*0.05f;
    m_Pose = m_Run.RenderPose(Manager::GetRenderAlpha());
    m_RenderOffset = m_Run.distance-m_Pose.distance;
    m_PathOrigin = RelicRun::SamplePath(0,m_Run.junctions,m_RenderOffset);
    auto* context = Renderer::GetDeviceContext();
    context->OMSetDepthStencilState(m_HudDepth,0);
    Renderer::SetWorldViewProjection2D();
    Rect(0,0,1280,720,{0.055f,0.17f,0.17f,1});
    Rect(0,230,1280,490,{0.12f,0.29f,0.26f,1});
    Flush();
    Renderer::SetDepthEnable(true);
    const float turn = RelicRun::TurnCameraStrength(m_Run.junctions,m_RenderOffset);
    const auto turnCamera = RelicRun::SampleTurnCamera(turn);
    const auto wallShake = m_WallFeedback.Sample(m_Run.phase == RelicRun::Phase::Playing ?
        Manager::GetRenderAlpha() : 1.0f);
    Renderer::SetProjectionMatrix(XMMatrixPerspectiveFovLH(turnCamera.fov,1280.0f/720.0f,0.2f,240.0f));
    const float shake = std::sin(m_Impact*95.0f)*m_Impact*0.28f;
    // Swing outside the corner while looking into it, with a pronounced bank.
    const auto eye = OnPath(m_Pose.x*0.12f+turnCamera.eyeX+shake+wallShake.x,
        turnCamera.eyeY+m_LandFlash*0.4f+wallShake.y,turnCamera.eyeZ);
    const auto target = OnPath(m_Pose.x*0.10f+turnCamera.targetX,1.0f,turnCamera.targetZ);
    const auto forward = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&target),XMLoadFloat3(&eye)));
    const auto right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0,1,0,0),forward));
    const auto levelUp = XMVector3Cross(forward,right);
    const float roll = turnCamera.roll+wallShake.roll;
    const auto up = XMVectorAdd(XMVectorScale(levelUp,std::cos(roll)),XMVectorScale(right,std::sin(roll)));
    Renderer::SetViewMatrix(XMMatrixLookAtLH(XMLoadFloat3(&eye),XMLoadFloat3(&target),up));
    m_WorldPass = true;
    DrawCourse();
    m_WorldPass = false;
    // Static placeholder: keep its feet and height aligned with the collider.
    const float fall = m_Run.death == RelicRun::Death::Fall ? (0.7f-m_Impact)*6.0f : 0.0f;
    Box(m_Pose.x,m_Pose.y+m_Pose.height*0.5f-fall,0,0.7f,m_Pose.height,0.6f,
        m_Impact>0?Gold:m_ActionFlash>0?Ivory:Teal);
    if (m_Impact > 0 && m_Run.death != RelicRun::Death::Fall)
    {
        for (int i = 0; i < 12; ++i)
        {
            const float t = 0.7f-m_Impact, angle = i*XM_2PI/12;
            Box(m_Pose.x+std::cos(angle)*t*3.5f,0.9f+std::sin(angle)*t*2.0f,t*2.0f,
                0.10f,0.10f,0.10f,Gold);
        }
    }
    Flush();
    context->OMSetDepthStencilState(m_HudDepth,0);
    Renderer::SetWorldViewProjection2D();
    DrawHud();
    Renderer::SetDepthEnable(true);
}
