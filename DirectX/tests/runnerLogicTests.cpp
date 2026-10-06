#include "../runnerLogic.h"
#include "../runnerCamera.h"
#include <iostream>
#include <stdexcept>

using namespace RelicRun;
void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
Run Course(Obstacle type)
{
    Run run;
    run.Reset();
    run.rows.clear(); run.coins.clear();
    Row row; row.z = 6.0f; row.obstacles[1] = type;
    run.rows.push_back(row);
    return run;
}
void Advance(Run& run, float seconds)
{
    for (int i = 0; i < static_cast<int>(seconds*60); ++i) run.Step(1.0f/60, {});
}
int main()
{
    try
    {
        Run run;
        Advance(run,1);
        Check(run.distance == 0, "Ready must not move");
        run.Reset();
        run.Step(1.0f/60, {-1}); Advance(run,0.3f);
        Check(run.lane == -1 && std::abs(run.x+3)<0.01f, "Left lane movement");
        run.Step(1.0f/60, {-1});
        Check(run.lane == -1, "Lane bounds");
        Check(run.events.wallHitDirection == -1 && run.phase == Phase::Playing && run.x == -3,
            "Outward left input gives feedback without moving or killing player");
        run.TogglePause(); const float distance = run.distance;
        const float pausedSpeed = run.speed;
        Advance(run,1); Check(run.distance == distance, "Pause freezes simulation");
        Check(run.speed == pausedSpeed, "Pause freezes acceleration");
        run.Step(1.0f/60,{-1}); Check(run.events.wallHitDirection == 0,"Paused input cannot bump wall");
        run.TogglePause(); Advance(run,0.1f); Check(run.distance > distance, "Resume moves");

        Run edges; edges.Reset();
        edges.Step(1.0f/60,{1});
        Check(edges.events.wallHitDirection == 0,"Entering outer lane is not a wall hit");
        Advance(edges,0.3f); edges.Step(1.0f/60,{1});
        Check(edges.events.wallHitDirection == 1 && edges.x == 3 && !edges.events.crash,
            "Right edge has directional nonlethal feedback");
        edges.Step(1.0f/60,{});
        Check(edges.events.wallHitDirection == 0,"Wall feedback does not repeat without new input");
        edges.Step(1.0f/60,{-1});
        Check(edges.events.wallHitDirection == 0 && edges.lane == 0,"Inward movement is unaffected");
        edges.phase = Phase::Dead; edges.lane = 1; edges.Step(1.0f/60,{1});
        Check(edges.events.wallHitDirection == 0,"Dead input cannot bump wall");
        edges.Reset(); Check(edges.events.wallHitDirection == 0,"Retry clears wall event");

        auto wall = Course(Obstacle::Wall); Advance(wall,0.8f);
        Check(wall.phase == Phase::Dead, "Wall kills");
        const auto deadDistance = wall.distance;
        const float deadSpeed = wall.speed;
        Advance(wall,1);
        Check(wall.distance == deadDistance, "Death freezes simulation");
        Check(wall.speed == deadSpeed, "Death freezes acceleration");
        wall.Reset(); Check(wall.phase == Phase::Playing && wall.distance == 0 && wall.coinCount == 0,
            "Retry resets run");
        auto dodge = Course(Obstacle::Wall);
        dodge.Step(1.0f/60, {1}); Advance(dodge,0.8f);
        Check(dodge.phase == Phase::Playing, "Lane change dodges wall");
        auto jump = Course(Obstacle::Hurdle);
        jump.Step(1.0f/60, {0,true});
        Check(jump.events.jump, "Jump sound event occurs once on accepted input");
        jump.Step(1.0f/60, {0,true}); Check(!jump.events.jump, "No repeated jump event in air");
        Advance(jump,0.8f);
        Check(jump.phase == Phase::Playing, "Jump clears hurdle");
        auto noJump = Course(Obstacle::Hurdle); Advance(noJump,0.8f);
        Check(noJump.phase == Phase::Dead, "Hurdle requires jump or dodge");
        auto slide = Course(Obstacle::Beam);
        slide.Step(1.0f/60, {0,false,true});
        Check(slide.events.slide, "Slide feedback event");
        Advance(slide,0.8f);
        Check(slide.phase == Phase::Playing, "Slide clears beam");
        auto noSlide = Course(Obstacle::Beam); Advance(noSlide,0.8f);
        Check(noSlide.phase == Phase::Dead, "Beam requires slide or dodge");
        auto swept = Course(Obstacle::Wall); swept.Step(0.6f,{});
        Check(swept.phase == Phase::Dead, "Swept collision cannot skip wall");
        Check(swept.events.crash, "Crash feedback event");
        swept.Step(1.0f/60,{}); Check(!swept.events.crash, "Crash sound must not repeat during death");
        Run pickup; pickup.Reset(); pickup.coins = {{0,0.8f,false}};
        Advance(pickup,0.2f);
        Check(pickup.coinCount == 1 && pickup.Score() >= 25, "Coin collected exactly once");

        auto gap = Course(Obstacle::Gap); gap.Step(1.0f/60,{0,true}); Advance(gap,0.8f);
        Check(gap.phase == Phase::Playing, "Jump clears gap");
        auto fall = Course(Obstacle::Gap); Advance(fall,0.8f);
        Check(fall.death == Death::Fall && fall.phase == Phase::Dead, "Gap causes fall");
        auto slideGap = Course(Obstacle::Gap); slideGap.Step(1.0f/60,{0,false,true}); Advance(slideGap,0.8f);
        Check(slideGap.death == Death::Fall, "Sliding cannot cross gap");
        auto sweptGap = Course(Obstacle::Gap); sweptGap.Step(0.6f,{});
        Check(sweptGap.death == Death::Fall, "Swept gap collision");

        Run interpolation; interpolation.Reset(); interpolation.Step(1.0f/60,{1,true});
        const auto p0 = interpolation.RenderPose(0), p1 = interpolation.RenderPose(1), mid = interpolation.RenderPose(0.5f);
        Check(p0.distance == 0 && std::abs(mid.distance-p1.distance*0.5f)<0.0001f,
            "Render distance interpolates between fixed ticks");
        Check(std::abs(mid.x-p1.x*0.5f)<0.0001f && std::abs(mid.y-p1.y*0.5f)<0.0001f,
            "Player horizontal and vertical interpolation");
        interpolation.TogglePause();
        Check(interpolation.RenderPose(0).distance == interpolation.distance, "Pause snaps to stable pose");
        interpolation.Reset(); Check(interpolation.RenderPose(0).distance == 0, "Retry resets interpolation");

        for (auto type : {Turn::Left,Turn::Right,Turn::Fork})
        {
            for (int choice : {-1,0,1})
            {
                Run corner; corner.Reset(); corner.rows.clear(); corner.coins.clear();
                corner.junctions = {{5,type,0,false}};
                corner.Step(1.0f/60,{choice}); Advance(corner,0.7f);
                const bool valid = choice != 0 && (type == Turn::Fork ||
                    (type == Turn::Left ? choice == -1 : choice == 1));
                Check((corner.phase == Phase::Playing) == valid, "Direction choice validation");
                if (!valid) Check(corner.death == Death::MissedTurn, "Missed corner reason");
            }
        }
        Run lateChoice; lateChoice.Reset(); lateChoice.rows.clear(); lateChoice.coins.clear();
        lateChoice.junctions = {{40,Turn::Left,0,false}};
        lateChoice.Step(1.0f/60,{-1});
        Check(lateChoice.junctions[0].choice == 0 && lateChoice.lane == -1, "Early input remains lane input");
        Advance(lateChoice,0.8f); lateChoice.Step(1.0f/60,{-1});
        Check(lateChoice.junctions[0].choice == -1 && lateChoice.lane == 0, "Turn zone buffers choice and centers player");
        Check(lateChoice.events.wallHitDirection == 0,"Turn selection at outer lane must not shake camera");

        const std::vector<Junction> path{{20,Turn::Right,1,false}};
        const auto before = SamplePath(20,path), after = SamplePath(20.001f,path), exit = SamplePath(20+12*3.14159265f/2,path);
        Check(std::abs(before.x-after.x)<0.002f && std::abs(before.z-after.z)<0.002f, "Bend position continuity");
        Check(std::abs(exit.angle-3.14159265f/2)<0.0001f && std::abs(exit.x-12)<0.001f,
            "Path makes a real 90 degree turn");
        const std::vector<Junction> fork{{20,Turn::Fork,0,false}};
        Check(SamplePath(50,fork,0,-1).x<0 && SamplePath(50,fork,0,1).x>0, "Fork branches diverge");

        Check(TurnCameraStrength({}) == 0 && TurnCameraStrength(path) == 0,"Straight camera stays level");
        for (int step = 0; step <= 800; ++step)
        {
            const float z = 20.0f-step*0.1f;
            const float left = TurnCameraStrength({{z,Turn::Left,-1,true}});
            const float right = TurnCameraStrength({{z,Turn::Right,1,true}});
            Check(left <= 0 && right >= 0 && right <= 1 && std::abs(left+right)<0.0001f,
                "Camera turn is bounded and mirrored for left and right");
            const float next = TurnCameraStrength({{z-0.1f,Turn::Right,1,true}});
            Check(std::abs(next-right)<0.01f,"Camera eases continuously through turn entry and exit");
            Check(std::abs(TurnCameraStrength({{z-0.1f,Turn::Right,1,true}},0.1f)-right)<0.0001f,
                "Turn camera respects render interpolation offset");
        }
        Check(TurnCameraStrength({{-9,Turn::Right,1,true}})>0.99f,"Camera reaches full swing mid-corner");
        Check(TurnCameraStrength({{-40,Turn::Right,1,true}}) == 0,"Camera settles after corner");
        Check(TurnCameraStrength({{0,Turn::Fork,-1,true}})<0 &&
            TurnCameraStrength({{0,Turn::Fork,1,true}})>0,"Camera follows selected fork direction");

        const auto straightCamera = SampleTurnCamera(0);
        Check(straightCamera.eyeX == 0 && straightCamera.eyeY == 6.8f && straightCamera.eyeZ == -12 &&
            straightCamera.targetX == 0 && straightCamera.targetZ == 9 && straightCamera.roll == 0 &&
            straightCamera.fov == 1.05f,"Stronger turn camera preserves straight view");
        for (int step = 0; step <= 100; ++step)
        {
            const float strength = step/100.0f;
            const auto leftCamera = SampleTurnCamera(-strength), rightCamera = SampleTurnCamera(strength);
            Check(leftCamera.eyeX == -rightCamera.eyeX && leftCamera.targetX == -rightCamera.targetX &&
                leftCamera.roll == -rightCamera.roll && leftCamera.eyeY == rightCamera.eyeY &&
                leftCamera.eyeZ == rightCamera.eyeZ && leftCamera.fov == rightCamera.fov,
                "Stronger camera is mirrored for both turns");
            Check(std::abs(rightCamera.eyeX)<=4 && std::abs(rightCamera.roll)<=0.20944f &&
                rightCamera.eyeY>=6.8f && rightCamera.eyeY<=8.11f && rightCamera.fov<=1.151f,
                "Stronger camera stays within motion limits");
        }
        const auto fullCamera = SampleTurnCamera(1);
        Check(fullCamera.eyeX == -4 && fullCamera.roll>0.209f && fullCamera.targetZ == 13,
            "Full turn has larger swing, twelve-degree bank and extended look ahead");
        Check(SampleTurnCamera(2).eyeX == fullCamera.eyeX && SampleTurnCamera(-2).roll == -fullCamera.roll,
            "Camera tuning clamps excessive input strength");

        WallCameraFeedback leftShake, rightShake;
        leftShake.Update(1.0f/60,-1,false); rightShake.Update(1.0f/60,1,false);
        Check(rightShake.Sample(1).x == 0,"Shake begins without a position jump");
        leftShake.Update(1.0f/60,0,false); rightShake.Update(1.0f/60,0,false);
        const auto hitLeft = leftShake.Sample(1), hitRight = rightShake.Sample(1);
        Check(hitRight.x > 0 && std::abs(hitLeft.x+hitRight.x)<0.0001f,
            "Wall shake initially kicks toward the input direction");
        Check(rightShake.Sample(0.5f).x > 0 && rightShake.Sample(0.5f).x < hitRight.x,
            "Shake interpolates between fixed updates");
        rightShake.Update(1.0f,0,true);
        Check(rightShake.Sample(1).x == hitRight.x,"Pause freezes wall shake");
        rightShake.Update(1.0f/60,1,false);
        rightShake.Update(1.0f/60,0,false);
        Check(std::abs(rightShake.Sample(1).x-hitRight.x)<0.0001f,"Repeated bumps retrigger without stacking");
        for (int frame = 0; frame < 60; ++frame)
        {
            rightShake.Update(1.0f/60,0,false);
            const auto sample = rightShake.Sample(0.5f);
            Check(std::abs(sample.x)<=0.45f && std::abs(sample.y)<=0.10f && std::abs(sample.roll)<=0.025f,
                "Wall shake stays within camera motion limits");
        }
        Check(rightShake.Sample(0).x == 0 && rightShake.Sample(1).y == 0 && rightShake.Sample(1).roll == 0,
            "Wall shake returns exactly to rest");
        leftShake.Reset(); Check(leftShake.Sample(0.5f).x == 0,"Retry clears interpolated shake");

        // At 144 Hz each rendered step should be smaller than one 60 Hz tick.
        Run highRefresh; highRefresh.Reset();
        double accumulator = 0.0;
        float lastDraw = 0.0f;
        for (int frame = 0; frame < 144; ++frame)
        {
            accumulator += 1.0/144.0;
            while (accumulator >= 1.0/60.0)
            { highRefresh.Step(1.0f/60,{}); accumulator -= 1.0/60.0; }
            const float draw = highRefresh.RenderPose(static_cast<float>(accumulator*60)).distance;
            Check(draw >= lastDraw && draw-lastDraw < 0.11f, "144 Hz interpolation moves smoothly");
            lastDraw = draw;
        }

        // Follow each guaranteed clear lane for ten simulated minutes.
        Run longRun; longRun.Reset();
        for (int frame = 0; frame < 36000; ++frame)
        {
            auto next = std::find_if(longRun.rows.begin(),longRun.rows.end(),
                [](const Row& row){return row.z > -1.5f;});
            int target = next == longRun.rows.end() ? 0 : next->safeLane-1;
            int command = target-longRun.lane;
            const auto* junction = longRun.NextJunction();
            if (junction && junction->z <= 32) command = junction->type == Turn::Right ? 1 : -1;
            const float previousSpeed = longRun.speed;
            longRun.Step(1.0f/60,{command});
            Check(longRun.speed >= previousSpeed && longRun.speed-previousSpeed <= Run::AccelerationPerSecond/60+0.0001f,
                "Acceleration is gradual and monotonic");
            if (frame == 1799) Check(std::abs(longRun.speed-20.0f)<0.001f,"Speed reaches 20 after 30 seconds");
            if (frame == 3599) Check(std::abs(longRun.speed-Run::MaximumSpeed)<0.001f,"Speed reaches cap after 60 seconds");
            Check(longRun.phase == Phase::Playing, "Generated course must be survivable");
            Check(longRun.rows.size() == 8 && longRun.coins.size() <= 60, "Bounded course memory");
            Check(longRun.junctions.size() == 3, "Bounded junction memory");
            for (const auto& row : longRun.rows)
                Check(row.obstacles[row.safeLane] == Obstacle::None, "Every row has an escape lane");
        }
        Check(longRun.speed == Run::MaximumSpeed && longRun.distance > 10000, "Speed cap and endless progression");
        const auto best = longRun.Score(); longRun.Reset();
        Check(longRun.bestScore == best, "Session best survives retry");
        Check(longRun.speed == Run::InitialSpeed, "Retry resets speed");
        longRun.Step(1.0f/60,{});
        Check(longRun.speed < Run::InitialSpeed+0.01f,"Retry resets acceleration clock");
        std::cout << "PASS: turn camera, wall shake, gradual acceleration, pause/retry, controls, feedback, interpolation, gaps, turns, forks, 10-minute course\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
