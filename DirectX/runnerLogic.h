#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// Simulation has no graphics or input-system dependencies, so collision and
// course generation can be verified without opening a window.
namespace RelicRun
{
enum class Phase { Ready, Playing, Paused, Dead };
enum class Obstacle { None, Wall, Hurdle, Beam, Gap };
enum class Turn { Left, Right, Fork };
enum class Death { None, Collision, Fall, MissedTurn };
struct Junction { float z{}; Turn type{}; int choice{}; bool resolved{}; };
struct Pose { float x{}, y{}, distance{}, height = 1.8f; };
inline float Lerp(float a, float b, float t) { return a + (b-a)*t; }
struct Events
{
    bool jump{}, slide{}, land{}, coin{}, crash{}, turn{}, warning{};
    int wallHitDirection{}; // -1: left edge, +1: right edge; feedback only.
};
struct Input { int laneDelta{}; bool jump{}; bool slide{}; };
struct Row
{
    float z{};
    std::array<Obstacle, 3> obstacles{};
    int safeLane{};
};
struct Coin { float x{}, z{}; bool collected{}; };

class Run
{
    uint32_t randomState = 0x57495245u;
    unsigned int rowNumber{};
    unsigned int junctionNumber{};
    Pose previousPose;
    double runningSeconds{};
    uint32_t Random()
    {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        return randomState;
    }
    void AddRow(float z)
    {
        Row row;
        row.z = z;
        row.safeLane = static_cast<int>(Random() % 3);
        for (int i = 0; i < 3; ++i)
        {
            if (i != row.safeLane)
                row.obstacles[i] = static_cast<Obstacle>(1 + Random() % 4);
        }
        // A gentle, reproducible opening introduces each silhouette separately.
        if (rowNumber < 4)
        {
            row.obstacles = {};
            row.obstacles[1] = rowNumber == 0 ? Obstacle::Hurdle :
                rowNumber == 1 ? Obstacle::Beam : rowNumber == 2 ? Obstacle::Wall : Obstacle::Gap;
            row.safeLane = rowNumber % 2 == 0 ? 0 : 2;
        }
        // Junction approaches are clear, leaving time to choose a direction.
        for (const auto& junction : junctions)
            if (std::abs(z-junction.z) < 36.0f) row.obstacles = {};
        rows.push_back(row);
        for (int i = 0; i < 5; ++i)
            coins.push_back({ (row.safeLane - 1) * LaneWidth, z - 9.0f + i * 2.0f, false });
        ++rowNumber;
    }
public:
    static constexpr float LaneWidth = 3.0f;
    static constexpr float InitialSpeed = 14.0f;
    static constexpr float MaximumSpeed = 26.0f;
    static constexpr float AccelerationPerSecond = 0.2f;
    Phase phase = Phase::Ready;
    int lane{}, coinCount{}, bestScore{};
    float x{}, y{}, verticalSpeed{}, slideTime{}, distance{}, speed = InitialSpeed;
    float collectFlash{};
    Death death = Death::None;
    Events events;
    std::vector<Row> rows;
    std::vector<Coin> coins;
    std::vector<Junction> junctions;

    Run() { Reset(false); }
    int Score() const { return static_cast<int>(distance) + coinCount * 25; }
    bool Sliding() const { return slideTime > 0.0f; }
    Pose CurrentPose() const { return {x,y,distance,Sliding()?0.7f:1.8f}; }
    Pose RenderPose(float alpha) const
    {
        if (phase != Phase::Playing) return CurrentPose();
        alpha = std::clamp(alpha,0.0f,1.0f);
        return {Lerp(previousPose.x,x,alpha),Lerp(previousPose.y,y,alpha),
            Lerp(previousPose.distance,distance,alpha),
            Lerp(previousPose.height,Sliding()?0.7f:1.8f,alpha)};
    }
    const Junction* NextJunction() const
    {
        for (const auto& junction : junctions) if (!junction.resolved) return &junction;
        return nullptr;
    }
    void Reset(bool start = true)
    {
        bestScore = std::max(bestScore, Score());
        phase = start ? Phase::Playing : Phase::Ready;
        lane = coinCount = 0;
        x = y = verticalSpeed = slideTime = distance = collectFlash = 0.0f;
        runningSeconds = 0.0;
        speed = InitialSpeed;
        death = Death::None;
        events = {};
        previousPose = CurrentPose();
        randomState = 0x57495245u;
        rowNumber = junctionNumber = 0;
        junctions.clear();
        for (int i = 0; i < 3; ++i)
        {
            junctions.push_back({160.0f+240.0f*i,static_cast<Turn>(junctionNumber%3),0,false});
            ++junctionNumber;
        }
        rows.clear(); coins.clear();
        for (int i = 0; i < 8; ++i) AddRow(40.0f + 24.0f * i);
        for (int i = 0; i < 7; ++i) coins.push_back({0.0f, 8.0f + i * 3.0f, false});
    }
    void TogglePause()
    {
        if (phase == Phase::Playing) phase = Phase::Paused;
        else if (phase == Phase::Paused) { phase = Phase::Playing; previousPose = CurrentPose(); }
    }
    void Step(float dt, const Input& input)
    {
        events = {};
        if (phase != Phase::Playing || dt <= 0.0f) return;
        previousPose = CurrentPose();
        bool choosingTurn = false;
        for (auto& junction : junctions)
        {
            if (!junction.resolved && junction.z <= 32.0f && junction.z >= 0.0f)
            {
                choosingTurn = true;
                if (input.laneDelta != 0) junction.choice = input.laneDelta < 0 ? -1 : 1;
                lane = 0;
                break;
            }
        }
        if (!choosingTurn)
        {
            if ((lane == -1 && input.laneDelta < 0) || (lane == 1 && input.laneDelta > 0))
                events.wallHitDirection = lane;
            lane = std::clamp(lane + input.laneDelta, -1, 1);
        }
        const float previousX = x;
        x += std::clamp(lane * LaneWidth - x, -20.0f * dt, 20.0f * dt);
        slideTime = std::max(0.0f, slideTime - dt);
        if (input.jump && y <= 0.001f && !Sliding()) { verticalSpeed = 10.5f; events.jump = true; }
        if (input.slide && y <= 0.001f && verticalSpeed <= 0.0f && !Sliding())
        { slideTime = 0.8f; events.slide = true; }
        const float previousY = y;
        verticalSpeed -= 26.0f * dt;
        y = std::max(0.0f, y + verticalSpeed * dt);
        if (y <= 0.0f) verticalSpeed = 0.0f;
        events.land = previousY > 0.0f && y == 0.0f;
        runningSeconds += dt;
        speed = std::min(MaximumSpeed,
            InitialSpeed + AccelerationPerSecond * static_cast<float>(runningSeconds));
        const float travel = speed * dt;
        distance += travel;
        collectFlash = std::max(0.0f, collectFlash - dt);
        for (auto& junction : junctions)
        {
            const float before = junction.z;
            junction.z -= travel;
            if (before > 50.0f && junction.z <= 50.0f) events.warning = true;
            if (!junction.resolved && junction.z <= 0.0f)
            {
                junction.resolved = true;
                const int required = junction.type == Turn::Left ? -1 : 1;
                if (junction.choice == 0 || (junction.type != Turn::Fork && junction.choice != required))
                { phase = Phase::Dead; death = Death::MissedTurn; events.crash = true; }
                else events.turn = true;
            }
        }
        junctions.erase(std::remove_if(junctions.begin(),junctions.end(),
            [](const Junction& j){return j.z < -100.0f;}),junctions.end());
        while (junctions.size() < 3)
        {
            junctions.push_back({junctions.empty()?160.0f:junctions.back().z+240.0f,
                static_cast<Turn>(junctionNumber%3),0,false});
            ++junctionNumber;
        }
        for (Row& row : rows)
        {
            const float previousZ = row.z;
            row.z -= travel;
            for (int i = 0; i < 3; ++i)
            {
                if (phase == Phase::Dead) continue;
                const auto obstacle = row.obstacles[i];
                const float extent = obstacle == Obstacle::Gap ? 1.8f : 1.1f;
                if (previousZ < -extent || row.z > extent) continue;
                const float hitTime = std::clamp((previousZ - extent) / travel, 0.0f, 1.0f);
                const float hitX = previousX + (x - previousX) * hitTime;
                const float feet = previousY + (y - previousY) * hitTime;
                if (std::abs(hitX - (i - 1) * LaneWidth) > 1.25f) continue;
                const bool hit = obstacle == Obstacle::Wall ||
                    (obstacle == Obstacle::Hurdle && feet < 1.15f) ||
                    (obstacle == Obstacle::Gap && feet < 0.2f) ||
                    (obstacle == Obstacle::Beam && feet + (Sliding() ? 0.7f : 1.8f) > 1.05f);
                if (hit)
                { phase = Phase::Dead; death = obstacle == Obstacle::Gap ? Death::Fall : Death::Collision; events.crash = true; }
            }
        }
        for (Coin& coin : coins)
        {
            const float previousZ = coin.z;
            coin.z -= travel;
            if (phase == Phase::Playing && !coin.collected && previousZ >= -0.7f && coin.z <= 0.7f &&
                std::abs(x - coin.x) < 0.95f && y < 1.4f)
            {
                coin.collected = true;
                ++coinCount;
                collectFlash = 0.2f;
                events.coin = true;
            }
        }
        rows.erase(std::remove_if(rows.begin(), rows.end(),
            [](const Row& row) { return row.z < -15.0f; }), rows.end());
        coins.erase(std::remove_if(coins.begin(), coins.end(),
            [](const Coin& coin) { return coin.z < -15.0f || coin.collected; }), coins.end());
        while (rows.size() < 8) AddRow(rows.empty() ? 40.0f : rows.back().z + 24.0f);
        bestScore = std::max(bestScore, Score());
    }
};
}
