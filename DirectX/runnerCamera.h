#pragma once
#include "runnerPath.h"

namespace RelicRun
{
// Signed curvature across the camera's view: ease in before a bend and out
// after it. Sampling the interpolated path keeps motion smooth at any refresh rate.
inline float TurnCameraStrength(const std::vector<Junction>& junctions, float renderOffset = 0.0f)
{
    const float bend = SamplePath(14.0f,junctions,renderOffset).angle -
        SamplePath(-14.0f,junctions,renderOffset).angle;
    const float t = std::clamp(std::abs(bend)/1.57079632679f,0.0f,1.0f);
    return std::copysign(t*t*(3.0f-2.0f*t),bend);
}

struct TurnCameraMotion
{
    float eyeX{}, eyeY{}, eyeZ{}, targetX{}, targetZ{}, roll{}, fov{};
};
inline TurnCameraMotion SampleTurnCamera(float strength)
{
    strength = std::clamp(strength,-1.0f,1.0f);
    const float amount = std::abs(strength);
    // Keep the stronger swing high enough to look over roadside pillars.
    return {-strength*4.0f,6.8f+amount*1.3f,-12.0f-amount*3.5f,
        strength*2.2f,9.0f+amount*4.0f,strength*0.20943951f,1.05f+amount*0.10f};
}

struct CameraShake { float x{}, y{}, roll{}; };
class WallCameraFeedback
{
    float remaining{}, previousRemaining{};
    int direction{};
public:
    static constexpr float Duration = 0.28f;
    void Reset() { remaining = previousRemaining = 0; direction = 0; }
    void Update(float dt, int hitDirection, bool paused)
    {
        previousRemaining = remaining;
        if (paused) return;
        remaining = std::max(0.0f,remaining-std::max(0.0f,dt));
        if (hitDirection != 0)
        {
            direction = hitDirection < 0 ? -1 : 1;
            // Retrigger instead of stacking amplitudes on repeated input.
            remaining = previousRemaining = Duration;
        }
    }
    CameraShake Sample(float alpha) const
    {
        const float time = Lerp(previousRemaining,remaining,std::clamp(alpha,0.0f,1.0f));
        if (time <= 0.0f) return {};
        const float elapsed = Duration-time;
        const float decay = (time/Duration)*(time/Duration);
        const float kick = direction*std::sin(elapsed*65.0f)*decay;
        return {kick*0.45f,std::sin(elapsed*90.0f)*decay*0.10f,kick*0.025f};
    }
};
}
