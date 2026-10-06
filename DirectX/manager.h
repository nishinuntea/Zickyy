#pragma once

class GameObject;//前方宣言
class Scene;
class Manager
{
private:
	static std::list<GameObject*>m_GameObjects;

	static Scene* m_Scene;
	static Scene* m_NextScene;
	static float m_ChangeTime;
	static float m_RenderAlpha;
	
public:
	static void Init(bool StartInGame = false);
	static void Uninit();
	static void Update(float DeltaTime);
	static void Draw(float Alpha = 1.0f);
	static float GetRenderAlpha() { return m_RenderAlpha; }

	template <typename T>//テンプレート関数
		static void ChangeScene(float Time=0.0f)
		{
			if(m_NextScene==nullptr)
			{
				m_ChangeTime = Time;
				m_NextScene = new T();
			}
		}
	
	template <typename T, typename... Args>//テンプレート関数
	static T* AddGameObject(Args&&... args)
	{
		std::unique_ptr<T> gameObject = std::make_unique<T>();
		try
		{
			gameObject->Init(std::forward<Args>(args)...);
		}
		catch (...)
		{
			gameObject->Uninit();
			throw;
		}
		T* result = gameObject.get();
		m_GameObjects.push_back(gameObject.release());

		return result;
	}


	template <typename T>//テンプレート関数
	static T* GetGameObject()
	{
		for (GameObject* gameObject : m_GameObjects)
		{
			T* find = dynamic_cast<T*>(gameObject);//RTTI(実行時型情報)
			if (find != nullptr)
				return find;
		}
		return nullptr;
	}


	template <typename T>
	static std::vector<T*> GetGameObjects()
	{
		std::vector<T*> gameObjects;
		for (GameObject* gameObject : m_GameObjects)
		{
			T* find = dynamic_cast<T*>(gameObject); //RTTI（実行時型情報）
			if (find != nullptr)
				gameObjects.push_back(find);
		}
		return gameObjects;
	}

};
