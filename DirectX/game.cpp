#include "main.h"
#include "renderer.h"
#include "manager.h"
#include "input.h"
#include "game.h"
#include "camera.h"
#include "field.h"
#include "player.h"
#include "sky.h"
#include "box.h"
#include "grappleAnchor.h"
#include "grappleVisual.h"
#include "runner.h"
#include "magneticCamera.h"
#include "magneticPrototype.h"

namespace
{
	void AddBuilding(const Vector3& Position, const Vector3& Scale)
	{
		Box* building = Manager::AddGameObject<Box>();
		building->SetPosition(Position);
		building->SetScale(Scale);

		const Vector3 top = building->GetCollisionMax();
		GrappleAnchor* anchor = Manager::AddGameObject<GrappleAnchor>();
		anchor->SetPosition({
			Position.x,
			top.y + 1.2f,
			Position.z
		});
	}
}

void Game::Init()
{
	Input::SetMouseLookEnabled(true);
	Manager::AddGameObject<MagneticCamera>();
	Manager::AddGameObject<MagneticPrototype>();
}

void WireGame::Init()
{
	Input::SetMouseLookEnabled(true);

	Manager::AddGameObject<Camera>();
	Manager::AddGameObject<Sky>();
	Manager::AddGameObject<Field>();

	AddBuilding({ -18.0f, 0.0f, -5.0f }, { 4.0f, 7.0f, 4.0f });
	AddBuilding({ 0.0f, 0.0f, 8.0f }, { 5.0f, 10.0f, 5.0f });
	AddBuilding({ 18.0f, 0.0f, 0.0f }, { 4.0f, 8.0f, 4.0f });
	AddBuilding({ -11.0f, 0.0f, 23.0f }, { 3.0f, 12.0f, 3.0f });
	AddBuilding({ 13.0f, 0.0f, 27.0f }, { 5.0f, 6.0f, 5.0f });
	AddBuilding({ 0.0f, 0.0f, 43.0f }, { 4.0f, 14.0f, 4.0f });
	AddBuilding({ -28.0f, 0.0f, 38.0f }, { 6.0f, 9.0f, 6.0f });
	AddBuilding({ 30.0f, 0.0f, 42.0f }, { 5.0f, 11.0f, 5.0f });

	Manager::AddGameObject<Player>();
	Manager::AddGameObject<GrappleVisual>();
}

void Game::Uninit()
{
	Input::SetMouseLookEnabled(false);
}

void Game::Update(float)
{
}

void Game::Draw()
{
}
