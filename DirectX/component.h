#pragma once



class Component
{

protected:

	class  GameObject* m_GameObject = nullptr;

public:
	Component() = delete;
	explicit Component(GameObject* Object) : m_GameObject(Object) {}
	virtual ~Component() = default;

	virtual void Init() {};
	virtual void Uninit() {};
	virtual void Update(float) {};
	virtual void Draw() {};

};
