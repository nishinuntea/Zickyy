#pragma once
#include "runnerLogic.h"

namespace RelicRun
{
struct PathPoint { float x{}, z{}, angle{}; };
// Circular 90-degree bends; longitudinal game distance is arc length.
inline PathPoint SamplePath(float distance, const std::vector<Junction>& junctions,
    float renderOffset = 0.0f, int forkOverride = 0)
{
    constexpr float pi = 3.14159265359f;
    constexpr float radius = 12.0f;
    PathPoint point{0,-160,0};
    float cursor = -160;
    auto straight = [&](float length)
    { point.x += std::sin(point.angle)*length; point.z += std::cos(point.angle)*length; };
    for (const auto& junction : junctions)
    {
        const float start = junction.z + renderOffset;
        if (distance <= start) break;
        straight(start-cursor);
        int direction = junction.type == Turn::Left ? -1 : 1;
        if (junction.type == Turn::Fork)
            direction = forkOverride != 0 && !junction.resolved ? forkOverride :
                junction.choice != 0 ? junction.choice : -1;
        const float arc = std::min(distance-start, radius*pi*0.5f);
        const float endAngle = point.angle + direction*arc/radius;
        point.x += radius/direction * (std::cos(point.angle)-std::cos(endAngle));
        point.z += radius/direction * (std::sin(endAngle)-std::sin(point.angle));
        point.angle = endAngle;
        cursor = start+arc;
        if (distance <= cursor) return point;
    }
    straight(distance-cursor);
    return point;
}
}
