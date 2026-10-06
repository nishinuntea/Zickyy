#pragma once

#include "Scene.h"
//#include "result.h"


class Result :public Scene
{
public:
	void Init()override;
	void Uninit()override;
	void Update(float DeltaTime)override;
	void Draw()override;

};
