#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "player.h"
#include "camera.h"
#include "box.h"
#include "grappleAnchor.h"
#include "audio.h"
#if defined(_WIN64)
#include "animationModel.h"
#endif

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace
{
	constexpr float PlayerRadius = 0.55f;
	constexpr float PlayerHeight = 1.8f;
	constexpr float MaxGrappleRange = 65.0f;
	constexpr float MinimumTargetAlignment = 0.35f;
	constexpr float AnimationTransitionDuration = 0.18f;
}

void Player::Init()
{
	m_Layer = 1;

#if defined(_WIN64)
	m_AnimationModel = AddComponent<AnimationModel>(this);
	if (!m_AnimationModel->Load(
		"asset\\model\\action_adventure\\X Bot.fbx"))
	{
		throw std::runtime_error(
			"Failed to load animated player model: X Bot.fbx");
	}

	const auto loadAnimation = [this](
		const char* FileName,
		const char* AnimationName)
	{
		if (!m_AnimationModel->LoadAnimation(FileName, AnimationName))
		{
			throw std::runtime_error(
				std::string("Failed to load player animation: ") + FileName);
		}
	};
	loadAnimation(
		"asset\\model\\action_adventure\\idle.fbx",
		"Idle");
	loadAnimation(
		"asset\\model\\action_adventure\\walking.fbx",
		"Walking");
	loadAnimation(
		"asset\\model\\action_adventure\\running.fbx",
		"Running");
	m_AnimationModel->SetLocalScale(0.01f);
#else
	ModelRenderer* modelRenderer = AddComponent<ModelRenderer>(this);
	modelRenderer->Load("asset\\model\\player.obj");
#endif

	Renderer::CreateVertexShader(
		&m_VertexShader,
		&m_VertexLayout,
		"shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(
		&m_PixelShader,
		"shader\\unlitTexturePS.cso");

	m_JumpSE = AddComponent<Audio>(this);
	m_JumpSE->Load("asset\\audio\\wan.wav");

	ResetMovement();
}

void Player::Uninit()
{
	DetachGrapple();
	m_AnimationModel = nullptr;

	SafeRelease(m_VertexLayout);
	SafeRelease(m_VertexShader);
	SafeRelease(m_PixelShader);

	GameObject::Uninit();
}

void Player::Update(float DeltaTime)
{
	if (Input::GetKeyTrigger('R'))
	{
		ResetMovement();
	}

	Camera* camera = Manager::GetGameObject<Camera>();
	if (camera == nullptr)
	{
		GameObject::Update(DeltaTime);
		return;
	}

	UpdateGrappleTarget(camera);

	const Vector3 movementInput = GetMovementInput(camera);
	UpdateGroundAndAirMovement(movementInput, DeltaTime);
	UpdateGrapple(DeltaTime);

	const float gravity = IsGrappling() ? -18.0f : -28.0f;
	m_Velocity.y += gravity * DeltaTime;

	const float maximumSpeed = IsGrappling()
		? (m_Boosting ? 68.0f : 50.0f)
		: (m_Grounded ? 18.0f : 40.0f);
	const float speed = m_Velocity.length();
	if (speed > maximumSpeed)
	{
		m_Velocity *= maximumSpeed / speed;
	}

	MoveWithCollisions(DeltaTime);

	const Vector3 horizontalVelocity =
		{ m_Velocity.x, 0.0f, m_Velocity.z };
	if (horizontalVelocity.length() > 0.5f)
	{
		m_Rotation.y = atan2f(
			horizontalVelocity.x,
			horizontalVelocity.z);
	}

	UpdateAnimation(DeltaTime);
	GameObject::Update(DeltaTime);
}

void Player::Draw()
{
	Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
	Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);

	GameObject::Draw();
}

Vector3 Player::GetMovementInput(Camera* CameraObject) const
{
	Vector3 forward = CameraObject->GetForward();
	Vector3 right = CameraObject->GetRight();
	forward.y = 0.0f;
	right.y = 0.0f;
	forward.normalize();
	right.normalize();

	Vector3 movement{};
	if (Input::GetKeyPress('W'))
	{
		movement += forward;
	}
	if (Input::GetKeyPress('S'))
	{
		movement -= forward;
	}
	if (Input::GetKeyPress('D'))
	{
		movement += right;
	}
	if (Input::GetKeyPress('A'))
	{
		movement -= right;
	}

	const float length = movement.length();
	if (length > 1.0f)
	{
		movement /= length;
	}
	return movement;
}

void Player::UpdateGrappleTarget(Camera* CameraObject)
{
	auto anchors = Manager::GetGameObjects<GrappleAnchor>();
	for (GrappleAnchor* anchor : anchors)
	{
		anchor->SetHighlighted(false);
		anchor->SetAttached(false);
	}

	if (m_GrappleAnchor != nullptr)
	{
		m_TargetAnchor = m_GrappleAnchor;
		m_GrappleAnchor->SetHighlighted(true);
		m_GrappleAnchor->SetAttached(true);
		return;
	}

	m_TargetAnchor = nullptr;

	Vector3 viewDirection = CameraObject->GetForward();
	viewDirection.y = 0.0f;
	viewDirection.normalize();

	float bestScore = std::numeric_limits<float>::lowest();
	const Vector3 playerCenter = m_Position + Vector3(0.0f, 1.0f, 0.0f);

	for (GrappleAnchor* anchor : anchors)
	{
		const Vector3 direction = anchor->GetPosition() - playerCenter;
		const float distance = direction.length();
		if (distance <= 0.001f || distance > MaxGrappleRange)
		{
			continue;
		}

		Vector3 horizontalDirection = direction;
		horizontalDirection.y = 0.0f;
		const float horizontalLength = horizontalDirection.length();
		if (horizontalLength <= 0.001f)
		{
			continue;
		}
		horizontalDirection /= horizontalLength;

		const float alignment = Vector3::dot(
			viewDirection,
			horizontalDirection);
		if (alignment < MinimumTargetAlignment)
		{
			continue;
		}

		const float score =
			alignment - (distance / MaxGrappleRange) * 0.18f;
		if (score > bestScore)
		{
			bestScore = score;
			m_TargetAnchor = anchor;
		}
	}

	if (m_TargetAnchor != nullptr)
	{
		m_TargetAnchor->SetHighlighted(true);
	}
}

void Player::UpdateGroundAndAirMovement(
	const Vector3& MovementInput,
	float DeltaTime)
{
	if (Input::GetKeyTrigger(VK_SPACE) && m_Grounded)
	{
		m_Velocity.y = 15.0f;
		m_Grounded = false;
		if (m_JumpSE != nullptr)
		{
			m_JumpSE->Play();
		}
	}

	if (m_Grounded && !IsGrappling())
	{
		const Vector3 desiredVelocity = MovementInput * 12.0f;
		const float response = std::min(1.0f, 10.0f * DeltaTime);
		m_Velocity.x +=
			(desiredVelocity.x - m_Velocity.x) * response;
		m_Velocity.z +=
			(desiredVelocity.z - m_Velocity.z) * response;
	}
	else
	{
		const float airAcceleration = IsGrappling() ? 19.0f : 12.0f;
		m_Velocity += MovementInput * airAcceleration * DeltaTime;

		const float airDrag = std::max(0.0f, 1.0f - 0.08f * DeltaTime);
		m_Velocity.x *= airDrag;
		m_Velocity.z *= airDrag;
	}
}

void Player::UpdateGrapple(float DeltaTime)
{
	const bool grappleHeld =
		Input::GetKeyPress('Z') ||
		Input::GetKeyPress(VK_LBUTTON);

	if (!grappleHeld)
	{
		DetachGrapple();
		return;
	}

	if (m_GrappleAnchor == nullptr)
	{
		if (m_TargetAnchor == nullptr)
		{
			return;
		}

		m_GrappleAnchor = m_TargetAnchor;
		m_GrappleAnchor->SetAttached(true);
		m_RopeLength = (
			m_GrappleAnchor->GetPosition() -
			(m_Position + Vector3(0.0f, 1.0f, 0.0f))).length();
	}

	const Vector3 playerCenter =
		m_Position + Vector3(0.0f, 1.0f, 0.0f);
	const Vector3 toAnchor =
		m_GrappleAnchor->GetPosition() - playerCenter;
	const float distance = toAnchor.length();
	if (distance <= 0.001f)
	{
		return;
	}

	Vector3 pullDirection = toAnchor / distance;
	m_RopeLength = std::max(4.0f, m_RopeLength - 18.0f * DeltaTime);
	const float stretch = std::max(0.0f, distance - m_RopeLength);
	const float pullAcceleration = 26.0f + stretch * 16.0f;
	m_Velocity += pullDirection * pullAcceleration * DeltaTime;

	m_Boosting = Input::GetKeyPress(VK_SHIFT);
	if (m_Boosting)
	{
		m_Velocity += pullDirection * 48.0f * DeltaTime;
	}
}

void Player::MoveWithCollisions(float DeltaTime)
{
	const Vector3 displacement = m_Velocity * DeltaTime;
	const int stepCount = std::clamp(
		static_cast<int>(ceilf(displacement.length() / 0.45f)),
		1,
		16);
	const Vector3 step = displacement / static_cast<float>(stepCount);

	m_Grounded = false;
	for (int index = 0; index < stepCount; ++index)
	{
		const Vector3 previousPosition = m_Position;
		Vector3 candidatePosition = m_Position + step;
		ResolveCollisions(previousPosition, candidatePosition);
		m_Position = candidatePosition;
	}
}

void Player::ResolveCollisions(
	const Vector3& PreviousPosition,
	Vector3& CandidatePosition)
{
	if (CandidatePosition.y <= 0.0f)
	{
		CandidatePosition.y = 0.0f;
		if (m_Velocity.y < 0.0f)
		{
			m_Velocity.y = 0.0f;
		}
		m_Grounded = true;
	}

	const auto buildings = Manager::GetGameObjects<Box>();
	for (Box* building : buildings)
	{
		Vector3 minimum = building->GetCollisionMin();
		Vector3 maximum = building->GetCollisionMax();
		minimum.x -= PlayerRadius;
		minimum.z -= PlayerRadius;
		maximum.x += PlayerRadius;
		maximum.z += PlayerRadius;

		const bool horizontalOverlap =
			CandidatePosition.x > minimum.x &&
			CandidatePosition.x < maximum.x &&
			CandidatePosition.z > minimum.z &&
			CandidatePosition.z < maximum.z;
		const bool verticalOverlap =
			CandidatePosition.y < maximum.y &&
			CandidatePosition.y + PlayerHeight > minimum.y;
		if (!horizontalOverlap || !verticalOverlap)
		{
			continue;
		}

		const bool landedOnRoof =
			PreviousPosition.y >= maximum.y - 0.08f &&
			CandidatePosition.y <= maximum.y &&
			m_Velocity.y <= 0.0f;
		if (landedOnRoof)
		{
			CandidatePosition.y = maximum.y;
			m_Velocity.y = 0.0f;
			m_Grounded = true;
			continue;
		}

		const float distanceToMinimumX =
			CandidatePosition.x - minimum.x;
		const float distanceToMaximumX =
			maximum.x - CandidatePosition.x;
		const float distanceToMinimumZ =
			CandidatePosition.z - minimum.z;
		const float distanceToMaximumZ =
			maximum.z - CandidatePosition.z;
		const float smallestPenetration = std::min(
			std::min(distanceToMinimumX, distanceToMaximumX),
			std::min(distanceToMinimumZ, distanceToMaximumZ));

		if (smallestPenetration == distanceToMinimumX)
		{
			CandidatePosition.x = minimum.x;
			m_Velocity.x = std::min(0.0f, m_Velocity.x);
		}
		else if (smallestPenetration == distanceToMaximumX)
		{
			CandidatePosition.x = maximum.x;
			m_Velocity.x = std::max(0.0f, m_Velocity.x);
		}
		else if (smallestPenetration == distanceToMinimumZ)
		{
			CandidatePosition.z = minimum.z;
			m_Velocity.z = std::min(0.0f, m_Velocity.z);
		}
		else
		{
			CandidatePosition.z = maximum.z;
			m_Velocity.z = std::max(0.0f, m_Velocity.z);
		}
	}

	const float clampedX = std::clamp(CandidatePosition.x, -58.0f, 58.0f);
	if (clampedX != CandidatePosition.x)
	{
		CandidatePosition.x = clampedX;
		m_Velocity.x = 0.0f;
	}

	const float clampedZ = std::clamp(CandidatePosition.z, -58.0f, 58.0f);
	if (clampedZ != CandidatePosition.z)
	{
		CandidatePosition.z = clampedZ;
		m_Velocity.z = 0.0f;
	}
}

void Player::UpdateAnimation(float DeltaTime)
{
	const float horizontalSpeed = sqrtf(
		m_Velocity.x * m_Velocity.x +
		m_Velocity.z * m_Velocity.z);

#if defined(_WIN64)
	if (m_AnimationModel != nullptr)
	{
		m_LocomotionAnimationElapsed += DeltaTime;
		LocomotionAnimation nextAnimation = m_LocomotionAnimation;

		if (!m_Grounded)
		{
			nextAnimation = LocomotionAnimation::Idle;
		}
		else
		{
			switch (m_LocomotionAnimation)
			{
			case LocomotionAnimation::Idle:
				if (horizontalSpeed > 0.75f)
				{
					nextAnimation = LocomotionAnimation::Walking;
				}
				break;
			case LocomotionAnimation::Walking:
				if (horizontalSpeed < 0.35f)
				{
					nextAnimation = LocomotionAnimation::Idle;
				}
				else if (
					horizontalSpeed > 8.0f &&
					m_LocomotionAnimationElapsed >= AnimationTransitionDuration)
				{
					nextAnimation = LocomotionAnimation::Running;
				}
				break;
			case LocomotionAnimation::Running:
				if (horizontalSpeed < 0.35f)
				{
					nextAnimation = LocomotionAnimation::Idle;
				}
				else if (horizontalSpeed < 6.0f)
				{
					nextAnimation = LocomotionAnimation::Walking;
				}
				break;
			}
		}

		if (nextAnimation != m_LocomotionAnimation)
		{
			m_LocomotionAnimation = nextAnimation;
			m_LocomotionAnimationElapsed = 0.0f;
		}

		const char* animationName = "Idle";
		switch (m_LocomotionAnimation)
		{
		case LocomotionAnimation::Walking:
			animationName = "Walking";
			break;
		case LocomotionAnimation::Running:
			animationName = "Running";
			break;
		case LocomotionAnimation::Idle:
		default:
			break;
		}
		m_AnimationModel->Update(animationName, DeltaTime);
		m_Scale = { 1.0f, 1.0f, 1.0f };
		return;
	}
#endif

	if (m_Grounded && horizontalSpeed > 0.5f)
	{
		m_MoveAnimation += horizontalSpeed * DeltaTime;
		const float animation = sinf(m_MoveAnimation * 2.5f);
		m_Scale.x = 1.0f - animation * 0.06f;
		m_Scale.y = 1.0f + animation * 0.1f;
		m_Scale.z = 1.0f - animation * 0.06f;
	}
	else
	{
		const float response = std::min(1.0f, 8.0f * DeltaTime);
		m_Scale += (Vector3(1.0f, 1.0f, 1.0f) - m_Scale) * response;
	}
}

void Player::DetachGrapple()
{
	if (m_GrappleAnchor != nullptr)
	{
		m_GrappleAnchor->SetAttached(false);
	}
	m_GrappleAnchor = nullptr;
	m_Boosting = false;
	m_RopeLength = 0.0f;
}

void Player::ResetMovement()
{
	DetachGrapple();

	const auto anchors = Manager::GetGameObjects<GrappleAnchor>();
	for (GrappleAnchor* anchor : anchors)
	{
		anchor->SetHighlighted(false);
		anchor->SetAttached(false);
	}

	m_Position = { 0.0f, 0.0f, -24.0f };
	m_Rotation = {};
	m_Scale = { 1.0f, 1.0f, 1.0f };
	m_Velocity = {};
	m_LocomotionAnimation = LocomotionAnimation::Idle;
	m_LocomotionAnimationElapsed = 0.0f;
	m_Grounded = true;
	m_MoveAnimation = 0.0f;
	m_TargetAnchor = nullptr;
}

Vector3 Player::GetGrapplePoint() const
{
	return m_GrappleAnchor != nullptr
		? m_GrappleAnchor->GetPosition()
		: m_Position;
}
