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
	SetWindowTextW(GetWindow(), L"RELIC RUN - Enter to start");
	Manager::AddGameObject<Runner>(false);
}
void Title::Uninit()
{

}
void Title::Update(float)
{
	if (Input::GetKeyTrigger(VK_F2))
	{
		Manager::ChangeScene<WireGame>();
	}
}
void Title::Draw()
{

}
