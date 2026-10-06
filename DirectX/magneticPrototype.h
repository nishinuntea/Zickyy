#pragma once

#include "gameObject.h"
#include "magnetLogic.h"

#include <string>
#include <vector>

class MagneticPrototype final : public GameObject
{
private:
	enum class BodyKind
	{
		Crate,
		Balloon,
		Enemy,
	};

	struct Body
	{
		BodyKind Kind{ BodyKind::Crate };
		Vector3 Position{};
		Vector3 PreviousPosition{};
		Vector3 SpawnPosition{};
		Vector3 Velocity{};
		Vector3 HalfExtent{ 0.5f, 0.5f, 0.5f };
		MagnetLogic::Polarity Polarity{ MagnetLogic::Polarity::North };
		bool Active{ true };
		bool Held{};
		bool Counted{};
		float RespawnRemaining{};
	};

	struct StaticBlock
	{
		Vector3 Position{};
		Vector3 HalfExtent{ 0.5f, 0.5f, 0.5f };
		XMFLOAT4 Color{ 0.5f, 0.5f, 0.5f, 1.0f };
		bool Magnetic{};
		MagnetLogic::Polarity Polarity{ MagnetLogic::Polarity::North };
	};

	ID3D11Buffer* m_VertexBuffer{};
	ID3D11InputLayout* m_VertexLayout{};
	ID3D11VertexShader* m_VertexShader{};
	ID3D11PixelShader* m_PixelShader{};

	std::vector<StaticBlock> m_StaticBlocks;
	std::vector<Body> m_Bodies;
	Vector3 m_PlayerPosition{};
	Vector3 m_PreviousPlayerPosition{};
	Vector3 m_PlayerVelocity{};
	const Vector3 m_PlayerHalfExtent{ 0.48f, 0.9f, 0.48f };
	MagnetLogic::Polarity m_PlayerPolarity{ MagnetLogic::Polarity::North };
	bool m_PlayerGrounded{};
	int m_HeldBodyIndex{ -1 };
	int m_CurrentFloor{ 1 };
	int m_Health{ 3 };
	int m_Score{};
	int m_BalloonsPopped{};
	float m_CoilTimer{};
	float m_DamageCooldown{};
	bool m_FloorComplete{};
	std::wstring m_LastCaption;

	void CreateCubeMesh();
	void BuildFloor(int floorNumber);
	void BuildFloorOne();
	void BuildFloorTwo();
	void BuildFloorThree();
	void BuildFloorFour();
	void AddRoom(float halfWidth, float halfDepth, float height);
	void AddStaticBlock(
		const Vector3& position,
		const Vector3& halfExtent,
		const XMFLOAT4& color,
		bool magnetic = false,
		MagnetLogic::Polarity polarity = MagnetLogic::Polarity::North);
	void AddBody(
		BodyKind kind,
		const Vector3& position,
		const Vector3& halfExtent,
		MagnetLogic::Polarity polarity);

	void HandleFloorSelection();
	void HandleMagnetInput();
	void UpdatePlayer(float deltaTime);
	void UpdateBodies(float deltaTime);
	void UpdateCrate(Body& body, float deltaTime);
	void UpdateBalloon(Body& body, float deltaTime);
	void UpdateEnemy(Body& body, float deltaTime);
	void ApplyMagneticSurfaceForces(float deltaTime);
	void ApplyCoilInterference(float deltaTime);
	void CheckFloorGoal();
	void UpdateCaption();

	bool ResolveStaticCollisions(
		Vector3& position,
		Vector3& velocity,
		const Vector3& halfExtent,
		bool* grounded) const;
	bool Overlaps(
		const Vector3& firstPosition,
		const Vector3& firstHalfExtent,
		const Vector3& secondPosition,
		const Vector3& secondHalfExtent) const;
	Vector3 GetViewForward() const;
	Vector3 GetViewRight() const;
	void ReleaseHeldBody(bool launch);

	void DrawCube(
		const Vector3& position,
		const Vector3& halfExtent,
		const XMFLOAT4& color) const;
	void DrawWorld() const;
	void DrawHud() const;

public:
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	Vector3 GetPlayerPosition() const { return m_PlayerPosition; }
	Vector3 GetPlayerVelocity() const { return m_PlayerVelocity; }
	int GetCurrentFloor() const { return m_CurrentFloor; }
};
