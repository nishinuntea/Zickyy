#pragma once
#include "scene.h"
class Game:public Scene
{
public:
	void Init()override;
	void Uninit()override;
	void Update(float DeltaTime)override;
	void Draw()override;

};

// The original grapple playground remains available from the runner menu.
class WireGame final : public Game
{
public:
    void Init() override;
};
