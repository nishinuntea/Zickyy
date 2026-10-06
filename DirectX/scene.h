#pragma once

class Scene
{public:
	virtual ~Scene() = default;
	virtual void Init(){}
	virtual void Uninit(){}
	virtual void Update(float){}
	virtual void Draw(){}



};
