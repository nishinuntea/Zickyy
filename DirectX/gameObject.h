#pragma once

#include "component.h"

class GameObject
{
protected://外部からアクセスできない、継承

	bool m_Destroy = false;

	int m_Layer = 1;//描画順番レイヤー番号

	float m_CameraZ = 0.0f;//ソート用Z値

    Vector3 m_Position{ 0.0f, 0.0f, 0.0f };
    Vector3 m_Rotation{ 0.0f, 0.0f, 0.0f };
    Vector3 m_Scale{ 1.0f, 1.0f, 1.0f };

	std::list<Component*>m_Components;

	GameObject* m_Parent = nullptr;
public:
	virtual ~GameObject() = default;

	int GetLayer() { return m_Layer; }

	float GetCameraZ() const { return m_CameraZ; }
	void CalcCameraZ(Vector3 CameraPosition, Vector3 CameraForward)
	{
		Vector3 direction = m_Position - CameraPosition;
		m_CameraZ = Vector3::dot(direction, CameraForward); //内積
	}

	void SetPosition(const Vector3& Position) { m_Position = Position; }
	Vector3 GetPosition() { return m_Position; }
	
	void SetRotation(const Vector3& Rotation) { m_Rotation = Rotation; }
	Vector3 GetRotation() { return m_Rotation; }

	void SetScale(const Vector3& Scale) { m_Scale = Scale; }
	Vector3 GetScale() { return m_Scale; }

	void SetParent(GameObject* Parent) { m_Parent = Parent; }

	XMMATRIX GetMatrix()
	{
		XMMATRIX world, scale, rot, trans;
		scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
		rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y + XM_PI, m_Rotation.z);
		trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
		world = scale * rot * trans;
	
		if (m_Parent)
		{
			world *= m_Parent->GetMatrix();
		}
		return world;

	
	}
	void SetDestroy() { m_Destroy = true;}

	bool IsDestroyed() const { return m_Destroy; }

	virtual void Init() {};
	virtual void Uninit()
	  {
		  for (Component* component : m_Components)
		  {
			  component->Uninit();
			  delete component;
		  }
		  m_Components.clear();
	  }
	virtual void Update(float DeltaTime)
	  {
		  for (Component* component : m_Components)
		  {
			  component->Update(DeltaTime);
		  }
	  }
	virtual void Draw()
	{
		XMMATRIX world = GetMatrix();
		Renderer::SetWorldMatrix(world);

		for (Component* component : m_Components)
		{
			component->Draw();
		}
	}
	template <typename T>//テンプレート関数
	T* AddComponent(GameObject*Object)
	  {
		  std::unique_ptr<T> component = std::make_unique<T>(Object);
		  try
		  {
			  component->Init();
		  }
		  catch (...)
		  {
			  component->Uninit();
			  throw;
		  }
		  T* result = component.get();
		  m_Components.push_back(component.release());

		  return result;
	  }
	virtual Vector3 GetForward()
	{
		XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
		
		Vector3 forward;
		XMStoreFloat3((XMFLOAT3*)&forward, rot.r[2]);
		return forward;
	}
	virtual Vector3 GetRight()
	{
		XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);

		Vector3 forward;
		XMStoreFloat3((XMFLOAT3*)&forward, rot.r[0]);
		return forward;
	}
};
