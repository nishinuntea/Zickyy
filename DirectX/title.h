#pragma once

#include "Scene.h"
//#include "title.h"


class Title :public Scene
{
public:
	void Init()override;
	void Uninit()override;
	void Update(float DeltaTime)override;
	void Draw()override;

};
