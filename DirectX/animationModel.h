#pragma once

#include <memory>
#include "component.h"
#include "vector3.h"

class AnimationModel : public Component
{
private:
	struct Impl;
	std::unique_ptr<Impl> m_Impl;

public:
	explicit AnimationModel(GameObject* gameObject);
	~AnimationModel() override;

	void Init() override;
	void Uninit() override;
	void Draw() override;

	bool Load(const char* fileName);
	bool LoadAnimation(const char* fileName, const char* animationName);
	void Update(const char* animationName, float deltaTime);

	bool IsLoaded() const;
	void SetLocalScale(float scale);
	void SetLocalRotation(const Vector3& rotation);
};
