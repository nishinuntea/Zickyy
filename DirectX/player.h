#pragma once

#include "gameObject.h"

class Audio;
class AnimationModel;
class Box;
class Camera;
class GrappleAnchor;

class Player final : public GameObject
{
private:
	enum class LocomotionAnimation
	{
		Idle,
		Walking,
		Running,
	};

	Vector3 m_Velocity{};

	ID3D11InputLayout* m_VertexLayout{};
	ID3D11VertexShader* m_VertexShader{};
	ID3D11PixelShader* m_PixelShader{};

	bool m_Grounded = true;
	bool m_Boosting{};
	float m_MoveAnimation{};
	LocomotionAnimation m_LocomotionAnimation = LocomotionAnimation::Idle;
	float m_LocomotionAnimationElapsed{};
	float m_RopeLength{};

	Audio* m_JumpSE{};
	AnimationModel* m_AnimationModel{};
	GrappleAnchor* m_TargetAnchor{};
	GrappleAnchor* m_GrappleAnchor{};

	Vector3 GetMovementInput(Camera* CameraObject) const;
	void UpdateGrappleTarget(Camera* CameraObject);
	void UpdateGroundAndAirMovement(
		const Vector3& MovementInput,
		float DeltaTime);
	void UpdateGrapple(float DeltaTime);
	void MoveWithCollisions(float DeltaTime);
	void ResolveCollisions(
		const Vector3& PreviousPosition,
		Vector3& CandidatePosition);
	void UpdateAnimation(float DeltaTime);
	void DetachGrapple();
	void ResetMovement();

public:
	void Init() override;
	void Uninit() override;
	void Update(float DeltaTime) override;
	void Draw() override;

	Vector3 GetVelocity() const { return m_Velocity; }
	float GetSpeed() const { return m_Velocity.length(); }

	bool HasGrappleTarget() const { return m_TargetAnchor != nullptr; }
	bool IsGrappling() const { return m_GrappleAnchor != nullptr; }
	bool IsBoosting() const { return m_Boosting; }
	Vector3 GetGrapplePoint() const;
};
