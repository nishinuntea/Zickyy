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
#include "result.h"
#include "title.h"



void Result::Init()
{
	Input::SetMouseLookEnabled(false);

	Manager::AddGameObject<Polygon2D>(
		0.0f,
		0.0f,
		static_cast<float>(SCREEN_WIDTH),
		static_cast<float>(SCREEN_HEIGHT),
		L"asset\\texture\\Result.png");
}
void Result::Uninit()
{

}
void Result::Update(float)
{
	if (Input::GetKeyTrigger(VK_RETURN))
	{
		Manager::ChangeScene<Title>();
	}
}
void Result::Draw()
{

}
