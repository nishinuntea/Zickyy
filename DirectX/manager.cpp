#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "gameObject.h"
#include "camera.h"
#include "game.h"
#include "title.h"
#include "Audio.h"
#include "modelRenderer.h"




//saticメンバ変数はcppファイルで定義する必要がある
std::list<GameObject*> Manager::m_GameObjects;
Scene* Manager::m_Scene=nullptr;
Scene* Manager::m_NextScene=nullptr;
float Manager::m_ChangeTime = 0.0f;
float Manager::m_RenderAlpha = 1.0f;

void Manager::Init(bool StartInGame)
{
    Input::Init();
    Renderer::Init();
    Audio::InitMaster();

    if (StartInGame)
    {
        ChangeScene<Game>();
    }
    else
    {
        ChangeScene<Title>();
    }
}



void Manager::Uninit()
{
    if (m_NextScene != nullptr)
    {
        delete m_NextScene;
        m_NextScene = nullptr;
    }

    if (m_Scene != nullptr)
    {
        m_Scene->Uninit();
        delete m_Scene;
        m_Scene = nullptr;
    }
	for (GameObject* gameObject : m_GameObjects)//範囲for文
	{
		gameObject->Uninit();
		delete gameObject;
    }
    m_GameObjects.clear();
    ModelRenderer::UnloadAll();
	Renderer::Uninit();
    Audio::UninitMaster();

    Input::Uninit();
}

void Manager::Update(float DeltaTime)
{
    Input::Update();
    //シーン切り替え
    if (m_NextScene != nullptr)
    {
        m_ChangeTime -= DeltaTime;
        if (m_ChangeTime <= 0.0f)
        {
            if (m_Scene != nullptr)
            {
                m_Scene->Uninit();
                delete m_Scene;
            }

            for (GameObject* gameObject : m_GameObjects)
            {
                gameObject->Uninit();
                delete gameObject;
            }

            m_GameObjects.clear();

            Scene* nextScene = m_NextScene;
            m_NextScene = nullptr;
            m_Scene = nextScene;
            m_Scene->Init();
        }

    }
    if (m_Scene != nullptr)
    {
        m_Scene->Update(DeltaTime);
    }
    //for (int i = 0; i < 3; i++)
    //{
    //	g_GameObjects[i]->Update();
    //}
    for (GameObject* gameObject : m_GameObjects)//範囲for文
    {
        gameObject->Update(DeltaTime);
    }

    //
    for (auto it = m_GameObjects.begin(); it != m_GameObjects.end();)
    {
        GameObject* gameObject = *it;
        if (gameObject->IsDestroyed())
        {
            gameObject->Uninit();
            delete gameObject;
            it = m_GameObjects.erase(it);
        }
        else
        {
            ++it;
        }
    }

}

void Manager::Draw(float Alpha)
{
    m_RenderAlpha = Alpha;
    Renderer::Begin();

    Camera* camera = GetGameObject<Camera>();


    if (camera)
    {
        //Z値計算
        Vector3 forward = camera->GetForward();
        Vector3 position = camera->GetPosition();

        for (GameObject* gameObject : m_GameObjects)
        {
            gameObject->CalcCameraZ(position, forward);
        }
        // Zソート（同じレイヤー内ではカメラから見て奥にある物から描画）
        m_GameObjects.sort([](GameObject* a, GameObject* b)
            {
                if (a->GetLayer() != b->GetLayer())
                {
                    return a->GetLayer() < b->GetLayer();
                }

                return a->GetCameraZ() > b->GetCameraZ();
            });
    }

    //描画
    for (int i = 0; i < 4; i++)
    {
        for (GameObject* gameObject : m_GameObjects)
        {
            if (gameObject->GetLayer() == i)
            {
                gameObject->Draw();
            }
        }
    }

    Renderer::End();
}
