#include "game.h"
#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "camera.h"
#include "sky.h"
#include "field.h" 
#include "polygon2D.h"
#include "player.h"
#include "enemy.h"
#include "bullet.h"
#include "tree.h"
#include "box.h"
#include "explosion.h"
#include "particle.h"
#include "title.h"
#include "runner.h"



void Title::Init()
{
	Input::SetMouseLookEnabled(false);

	//謎のランゲームが始まる
	//SetWindowTextW(GetWindow(), L"RELIC RUN - Enter to start");
	//Manager::AddGameObject<Runner>(false);

	//タイトル表示
	Manager::AddGameObject<Polygon2D>(
		0.0f,
		0.0f,
		static_cast<float>(SCREEN_WIDTH),
		static_cast<float>(SCREEN_HEIGHT),
		L"asset\\texture\\Title1.png");
}
void Title::Uninit()
{

}
void Title::Update(float)
{
	if (Input::GetKeyTrigger(VK_RETURN))
	{
		Manager::ChangeScene<Game>();
	}
}
void Title::Draw()
{

}
