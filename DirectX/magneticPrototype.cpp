#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "magneticCamera.h"
#include "magneticPrototype.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <limits>

namespace
{
	// 円周上の配置計算に使う円周率。
	constexpr float Pi = 3.14159265358979323846f;
	// 磁力と箱取得の有効半径。
	constexpr float MagnetRadius = 3.0f;
	// 入力時に目標とする水平移動速度（毎秒）。
	constexpr float PlayerMoveSpeed = 7.0f;
	// 重力加速度。Yのマイナスが下方向。
	constexpr float Gravity = -20.0f;
	// HUD用バッファの上限頂点数。四角形を追加するときに超過を防ぐ。
	constexpr size_t MaxHudVertices = 65536;

	// N極のRGBA色。各成分は0～1で、最後の値は不透明度。
	XMFLOAT4 NorthColor()
	{
		return { 0.12f, 0.65f, 1.0f, 1.0f };
	}

	// S極のRGBA色。
	XMFLOAT4 SouthColor()
	{
		return { 1.0f, 0.22f, 0.25f, 1.0f };
	}

	// 極性から表示色を選ぶ。
	XMFLOAT4 PolarityColor(MagnetLogic::Polarity polarity)
	{
		return polarity == MagnetLogic::Polarity::North ? NorthColor() : SouthColor();
	}

	// 1文字を7行のビット列で表す。各行の下位5ビットが左から右の点に対応する。
	// 例：0x0E = 01110。1の部分だけ塗る。未対応の文字はすべて0となり空白になる。
	std::array<unsigned char, 7> HudGlyph(char character)
	{
		switch (character)
		{
		case 'A': return { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 };
		case 'B': return { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E };
		case 'C': return { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E };
		case 'D': return { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E };
		case 'E': return { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F };
		case 'F': return { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 };
		case 'G': return { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F };
		case 'H': return { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 };
		case 'I': return { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F };
		case 'J': return { 0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C };
		case 'K': return { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 };
		case 'L': return { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F };
		case 'M': return { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 };
		case 'N': return { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 };
		case 'O': return { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E };
		case 'P': return { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 };
		case 'Q': return { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D };
		case 'R': return { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 };
		case 'S': return { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E };
		case 'T': return { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 };
		case 'U': return { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E };
		case 'V': return { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 };
		case 'W': return { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A };
		case 'X': return { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 };
		case 'Y': return { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 };
		case 'Z': return { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F };
		case '0': return { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E };
		case '1': return { 0x04, 0x0C, 0x14, 0x04, 0x04, 0x04, 0x1F };
		case '2': return { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F };
		case '3': return { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E };
		case '4': return { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 };
		case '5': return { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E };
		case '6': return { 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E };
		case '7': return { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 };
		case '8': return { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E };
		case '9': return { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E };
		case ':': return { 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 };
		case '-': return { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 };
		case '/': return { 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10 };
		case '.': return { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C };
		default: return {};
		}
	}
}

// 初期化：描画用の頂点バッファとシェーダーを作り、1階の配置を生成する。
void MagneticPrototype::Init()
{
	m_Layer = 1;
	CreateCubeMesh();
	CreateHudBuffer();
	Renderer::CreateVertexShader(
		&m_VertexShader,
		&m_VertexLayout,
		"shader\\magnetToonVS.cso");
	Renderer::CreatePixelShader(
		&m_PixelShader,
		"shader\\magnetToonPS.cso");
	Renderer::CreatePixelShader(
		&m_HudPixelShader,
		"shader\\hudColorPS.cso");
	BuildFloor(1);
}

// 終了処理：DirectXの参照を解放し、物体データと親クラスのコンポーネントを片付ける。
void MagneticPrototype::Uninit()
{
	SafeRelease(m_VertexBuffer);
	SafeRelease(m_VertexLayout);
	SafeRelease(m_VertexShader);
	SafeRelease(m_PixelShader);
	SafeRelease(m_HudPixelShader);
	SafeRelease(m_HudVertexBuffer);
	// 今回のHUDを一から組み立てるため、前回分の頂点を空にする。
	m_HudVertices.clear();
	m_StaticBlocks.clear();
	m_Bodies.clear();
	GameObject::Uninit();
}

// 立方体の形を作る。6面 × 各2三角形 × 3頂点 = 36頂点。座標は各軸−1～1。
void MagneticPrototype::CreateCubeMesh()
{
	std::array<VERTEX_3D, 36> vertices{};
	size_t cursor = 0;
	// ラムダ式（その場で定義する小さな関数）で1頂点を配列へ追加する。法線は面の向き。
	const auto addVertex = [&vertices, &cursor](const Vector3& position, const Vector3& normal)
		{
			VERTEX_3D vertex{};
			vertex.Position = { position.x, position.y, position.z };
			vertex.Normal = { normal.x, normal.y, normal.z };
			vertex.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
			vertex.TexCoord = { 0.0f, 0.0f };
			vertices[cursor++] = vertex;
		};
	// 四角い1面を2つの三角形へ分割する。面ごとに同じ法線を設定する。
	const auto addFace = [&addVertex](
		const Vector3& first,
		const Vector3& second,
		const Vector3& third,
		const Vector3& fourth,
		const Vector3& normal)
		{
			addVertex(first, normal);
			addVertex(second, normal);
			addVertex(third, normal);
			addVertex(first, normal);
			addVertex(third, normal);
			addVertex(fourth, normal);
		};

	addFace({ -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f });
	addFace({ 1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f }, { -1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f });
	addFace({ -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, -1.0f }, { -1.0f, 0.0f, 0.0f });
	addFace({ 1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f });
	addFace({ -1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, -1.0f }, { -1.0f, 1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f });
	addFace({ -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, 1.0f }, { -1.0f, -1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f });

	D3D11_BUFFER_DESC description{};
	description.Usage = D3D11_USAGE_DEFAULT;
	// バッファの容量をバイト数で指定する。
	description.ByteWidth = static_cast<UINT>(sizeof(VERTEX_3D) * vertices.size());
	description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA data{};
	// CPU上の頂点配列を、バッファ作成時の初期データとして渡す。
	data.pSysMem = vertices.data();
	ThrowIfFailed(
		Renderer::GetDevice()->CreateBuffer(&description, &data, &m_VertexBuffer),
		"Create magnetic cube vertex buffer");
}

// HUD（画面上の文字や枠）用の頂点バッファを作る。毎回CPUから書き換えるためDYNAMICを使う。
void MagneticPrototype::CreateHudBuffer()
{
	D3D11_BUFFER_DESC description{};
	description.Usage = D3D11_USAGE_DYNAMIC;
	// バッファの容量をバイト数で指定する。
	description.ByteWidth = static_cast<UINT>(sizeof(VERTEX_3D) * MaxHudVertices);
	description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ThrowIfFailed(
		Renderer::GetDevice()->CreateBuffer(&description, nullptr, &m_HudVertexBuffer),
		"Create magnetic HUD vertex buffer");
	m_HudVertices.reserve(MaxHudVertices);
}

// 指定階を作り直す。HP・得点・極性・保持中の箱・タイマーも初期値へ戻す。
void MagneticPrototype::BuildFloor(int floorNumber)
{
	// 範囲外の階番号を1～4に収める。
	m_CurrentFloor = std::clamp(floorNumber, 1, 4);
	m_StaticBlocks.clear();
	m_Bodies.clear();
	m_PlayerVelocity = {};
	m_PlayerPolarity = MagnetLogic::Polarity::North;
	// 接地状態は、このあとの衝突処理で再判定する。
	m_PlayerGrounded = false;
	// −1は「保持している箱なし」を表す。
	m_HeldBodyIndex = -1;
	m_Health = 3;
	m_Score = 0;
	m_BalloonsPopped = 0;
	m_CoilTimer = 0.0f;
	m_DamageCooldown = 0.0f;
	// 開始説明を表示する残り秒数。
	m_FloorIntroRemaining = 6.0f;
	m_PolarityFlashRemaining = 0.0f;
	m_FloorComplete = false;

	switch (m_CurrentFloor)
	{
	case 1:
		BuildFloorOne();
		break;
	case 2:
		BuildFloorTwo();
		break;
	case 3:
		BuildFloorThree();
		break;
	case 4:
		BuildFloorFour();
		break;
	default:
		break;
	}
	m_PreviousPlayerPosition = m_PlayerPosition;
	UpdateCaption();
}

// 床・天井・左右・前後の6つの固定直方体で部屋を作る。幅と奥行きは半分の長さを受け取る。
void MagneticPrototype::AddRoom(float halfWidth, float halfDepth, float height)
{
	const XMFLOAT4 floorColor{ 0.18f, 0.22f, 0.28f, 1.0f };
	const XMFLOAT4 wallColor{ 0.28f, 0.32f, 0.40f, 1.0f };
	AddStaticBlock({ 0.0f, -0.5f, 0.0f }, { halfWidth, 0.5f, halfDepth }, floorColor);
	AddStaticBlock({ 0.0f, height + 0.5f, 0.0f }, { halfWidth, 0.5f, halfDepth }, wallColor);
	AddStaticBlock({ -halfWidth - 0.5f, height * 0.5f, 0.0f }, { 0.5f, height * 0.5f, halfDepth }, wallColor);
	AddStaticBlock({ halfWidth + 0.5f, height * 0.5f, 0.0f }, { 0.5f, height * 0.5f, halfDepth }, wallColor);
	AddStaticBlock({ 0.0f, height * 0.5f, -halfDepth - 0.5f }, { halfWidth, height * 0.5f, 0.5f }, wallColor);
	AddStaticBlock({ 0.0f, height * 0.5f, halfDepth + 0.5f }, { halfWidth, height * 0.5f, 0.5f }, wallColor);
}

// 1階の配置データを作る。箱を5列×2行、風船を5か所に登録する。
void MagneticPrototype::BuildFloorOne()
{
	AddRoom(12.0f, 12.0f, 10.0f);
	m_PlayerPosition = { 0.0f, 0.95f, -8.5f };

	for (int index = 0; index < 10; ++index)
	{
		// 余りで横の列番号を、整数の割り算で縦の行番号を求める。
		const int column = index % 5;
		const int row = index / 5;
		AddBody(
			BodyKind::Crate,
			{ -8.0f + static_cast<float>(column) * 4.0f, 0.7f, -1.5f + static_cast<float>(row) * 4.0f },
			{ 0.7f, 0.7f, 0.7f },
			index % 2 == 0 ? MagnetLogic::Polarity::North : MagnetLogic::Polarity::South);
	}

	const std::array<Vector3, 5> balloonPositions = {
		Vector3{ -9.0f, 1.2f, 8.0f },
		Vector3{ -4.5f, 1.2f, 9.0f },
		Vector3{ 0.0f, 1.2f, 8.5f },
		Vector3{ 4.5f, 1.2f, 9.0f },
		Vector3{ 9.0f, 1.2f, 8.0f },
	};
	for (const Vector3& position : balloonPositions)
	{
		AddBody(BodyKind::Balloon, position, { 0.75f, 1.0f, 0.75f }, MagnetLogic::Polarity::North);
	}
}

// 2階の配置データを作る。N極の床・天井と、S極の側面パネルを登録する。
void MagneticPrototype::BuildFloorTwo()
{
	m_PlayerPosition = { 0.0f, 1.0f, -21.0f };
	const XMFLOAT4 neutral{ 0.22f, 0.25f, 0.32f, 1.0f };
	AddStaticBlock({ 0.0f, -0.5f, 0.0f }, { 5.0f, 0.5f, 25.0f }, NorthColor(), true, MagnetLogic::Polarity::North);
	AddStaticBlock({ 0.0f, 10.5f, 0.0f }, { 5.0f, 0.5f, 25.0f }, NorthColor(), true, MagnetLogic::Polarity::North);
	AddStaticBlock({ 0.0f, 5.0f, -25.5f }, { 5.5f, 5.0f, 0.5f }, neutral);
	AddStaticBlock({ 0.0f, 5.0f, 25.5f }, { 5.5f, 5.0f, 0.5f }, neutral);
	AddStaticBlock({ -5.5f, 5.0f, 0.0f }, { 0.5f, 5.0f, 25.0f }, neutral);
	AddStaticBlock({ 5.5f, 5.0f, 0.0f }, { 0.5f, 5.0f, 25.0f }, neutral);

	for (float z : { -14.0f, 0.0f, 14.0f })
	{
		AddStaticBlock({ -5.0f, 5.0f, z }, { 0.15f, 4.0f, 2.5f }, SouthColor(), true, MagnetLogic::Polarity::South);
		AddStaticBlock({ 5.0f, 5.0f, z }, { 0.15f, 4.0f, 2.5f }, SouthColor(), true, MagnetLogic::Polarity::South);
	}
	AddStaticBlock({ 0.0f, 0.4f, 22.0f }, { 3.0f, 0.4f, 1.5f }, { 0.2f, 0.9f, 0.35f, 1.0f });
}

// 3階の配置データを作る。円周上の敵4体、箱4個、出口の台を登録する。
void MagneticPrototype::BuildFloorThree()
{
	AddRoom(13.0f, 13.0f, 10.0f);
	m_PlayerPosition = { 0.0f, 0.95f, -9.0f };
	for (int index = 0; index < 4; ++index)
	{
		// 90度ずつずらし、cos・sinで円周上のX・Z座標を求める。
		const float angle = static_cast<float>(index) * Pi * 0.5f;
		AddBody(
			BodyKind::Enemy,
			{ std::cos(angle) * 8.0f, 0.8f, std::sin(angle) * 8.0f },
			{ 0.8f, 0.8f, 0.8f },
			index % 2 == 0 ? MagnetLogic::Polarity::North : MagnetLogic::Polarity::South);
	}
	for (int index = 0; index < 4; ++index)
	{
		AddBody(
			BodyKind::Crate,
			{ -4.5f + static_cast<float>(index) * 3.0f, 0.65f, 6.0f },
			{ 0.65f, 0.65f, 0.65f },
			index % 2 == 0 ? MagnetLogic::Polarity::North : MagnetLogic::Polarity::South);
	}
	AddStaticBlock({ 0.0f, 0.35f, 10.0f }, { 2.0f, 0.35f, 1.0f }, { 0.2f, 0.9f, 0.35f, 1.0f });
}

// 4階の配置データを作る。箱8個と高い出口の台を登録する。
void MagneticPrototype::BuildFloorFour()
{
	AddRoom(12.0f, 18.0f, 14.0f);
	m_PlayerPosition = { 0.0f, 0.95f, -13.0f };
	for (int index = 0; index < 8; ++index)
	{
		// 余りで横の列番号を、整数の割り算で縦の行番号を求める。
		const int column = index % 4;
		const int row = index / 4;
		AddBody(
			BodyKind::Crate,
			{ -4.5f + static_cast<float>(column) * 3.0f, 0.75f, -5.0f + static_cast<float>(row) * 3.0f },
			{ 1.2f, 0.75f, 1.2f },
			index % 2 == 0 ? MagnetLogic::Polarity::North : MagnetLogic::Polarity::South);
	}
	AddStaticBlock({ 0.0f, 5.0f, 11.5f }, { 4.0f, 0.5f, 3.5f }, { 0.2f, 0.9f, 0.35f, 1.0f });
	AddStaticBlock({ 0.0f, 2.5f, 14.5f }, { 4.0f, 2.5f, 0.5f }, { 0.25f, 0.32f, 0.38f, 1.0f });
}

// 動かない直方体を一覧へ追加する。positionは中心、halfExtentは各軸の大きさの半分。
void MagneticPrototype::AddStaticBlock(
	const Vector3& position,
	const Vector3& halfExtent,
	const XMFLOAT4& color,
	bool magnetic,
	MagnetLogic::Polarity polarity)
{
	StaticBlock block{};
	block.Position = position;
	// 中心から端までの長さ。全体寸法はこの値の2倍。
	block.HalfExtent = halfExtent;
	block.Color = color;
	block.Magnetic = magnetic;
	block.Polarity = polarity;
	// 固定物一覧に値をコピーして登録する。
	m_StaticBlocks.push_back(block);
}

// 箱・風船・敵を一覧へ追加する。復活位置も初期位置と同じ値で保存する。
void MagneticPrototype::AddBody(
	BodyKind kind,
	const Vector3& position,
	const Vector3& halfExtent,
	MagnetLogic::Polarity polarity)
{
	Body body{};
	body.Kind = kind;
	body.Position = position;
	body.PreviousPosition = position;
	body.SpawnPosition = position;
	body.HalfExtent = halfExtent;
	body.Polarity = polarity;
	// 動的な物体一覧に値をコピーして登録する。
	m_Bodies.push_back(body);
}

// 更新処理の入口。deltaTimeは今回進める秒数。入力→力→移動→判定→表示情報の順に更新する。
void MagneticPrototype::Update(float deltaTime)
{
	HandleFloorSelection();
	// GetKeyTriggerは押した瞬間だけtrue。ヘルプ表示を反転する。
	if (Input::GetKeyTrigger('H'))
	{
		m_HelpVisible = !m_HelpVisible;
	}
	// 現在の階を再生成し、この回の更新を終了する。
	if (Input::GetKeyTrigger('R'))
	{
		BuildFloor(m_CurrentFloor);
		return;
	}
	// クリア後は次の階へ。4階の次は1階に戻る。Game側にもEnter処理がある点に注意。
	if (m_FloorComplete && Input::GetKeyTrigger(VK_RETURN))
	{
		BuildFloor(m_CurrentFloor == 4 ? 1 : m_CurrentFloor + 1);
		return;
	}

	m_PreviousPlayerPosition = m_PlayerPosition;
	// 残り時間を減らし、負の値にはしない。ダメージ後は一定時間、再ダメージを防ぐ。
	m_DamageCooldown = std::max(0.0f, m_DamageCooldown - deltaTime);
	m_FloorIntroRemaining = std::max(0.0f, m_FloorIntroRemaining - deltaTime);
	m_PolarityFlashRemaining = std::max(0.0f, m_PolarityFlashRemaining - deltaTime);
	HandleMagnetInput();
	ApplyCoilInterference(deltaTime);
	ApplyMagneticSurfaceForces(deltaTime);
	UpdatePlayer(deltaTime);
	UpdateBodies(deltaTime);
	CheckFloorGoal();
	UpdateCaption();
	// 親クラスの更新を呼び、取り付けられたコンポーネントも更新する。
	GameObject::Update(deltaTime);
}

// 1～4キーの「押した瞬間」を調べ、対応する階を作り直す。
void MagneticPrototype::HandleFloorSelection()
{
	for (int floor = 1; floor <= 4; ++floor)
	{
		if (Input::GetKeyTrigger(static_cast<BYTE>('0' + floor)))
		{
			BuildFloor(floor);
			return;
		}
	}
}

// Vで極性変更、Cで取得／解除、Bで投射または磁力パルスを処理する。
void MagneticPrototype::HandleMagnetInput()
{
	// NとSを反転し、HUDの極性表示を短時間強調する。
	if (Input::GetKeyTrigger('V'))
	{
		m_PlayerPolarity = MagnetLogic::Opposite(m_PlayerPolarity);
		m_PolarityFlashRemaining = 0.3f;
	}

	if (Input::GetKeyTrigger('C'))
	{
		if (m_HeldBodyIndex >= 0)
		{
			ReleaseHeldBody(false);
		}
		else
		{
			// 取得できる箱を探す。見つからなければ−1。
			const int closestIndex = FindGrabbableBodyIndex();
			if (closestIndex >= 0)
			{
				m_HeldBodyIndex = closestIndex;
				m_Bodies[static_cast<size_t>(closestIndex)].Held = true;
				m_Bodies[static_cast<size_t>(closestIndex)].Velocity = {};
			}
		}
	}

	if (Input::GetKeyTrigger('B'))
	{
		if (m_HeldBodyIndex >= 0)
		{
			ReleaseHeldBody(true);
			return;
		}

		for (Body& body : m_Bodies)
		{
			if (!body.Active || body.Kind == BodyKind::Balloon)
			{
				continue;
			}
			// 主人公から物体へ向かうベクトル。長さが距離になる。
			Vector3 direction = body.Position - m_PlayerPosition;
			const float distance = direction.length();
			if (distance <= 0.001f || distance > MagnetRadius)
			{
				continue;
			}
			// 距離で割って長さ1の方向ベクトルにする。ゼロ距離は事前に除外する。
			direction /= distance;
			// 方向が主人公→物体なので、引力ならマイナス、反発ならプラスを掛ける。
			const float polarityDirection = -MagnetLogic::ForceDirection(m_PlayerPolarity, body.Polarity);
			const float impulse = MagnetLogic::ForceMagnitude(distance, MagnetRadius, 24.0f);
			// パルスは一瞬の速度変化として加えるため、ここではdeltaTimeを掛けない。
			body.Velocity += direction * (polarityDirection * impulse);
		}
	}
}

// 保持状態を解除する。launchがtrueなら水平方向の前方速度と上向き速度を与える。
void MagneticPrototype::ReleaseHeldBody(bool launch)
{
	if (m_HeldBodyIndex < 0 || static_cast<size_t>(m_HeldBodyIndex) >= m_Bodies.size())
	{
		// −1は「保持している箱なし」を表す。
		m_HeldBodyIndex = -1;
		return;
	}

	Body& body = m_Bodies[static_cast<size_t>(m_HeldBodyIndex)];
	body.Held = false;
	if (launch)
	{
		// カメラの水平前方へ投げ、上向き速度も足して弧を描かせる。
		body.Velocity = GetViewForward() * 19.0f + Vector3(0.0f, 2.5f, 0.0f);
	}
	// −1は「保持している箱なし」を表す。
	m_HeldBodyIndex = -1;
}

// カメラ基準の移動入力から速度を求め、ジャンプ・重力・位置更新・固定物との衝突を処理する。
void MagneticPrototype::UpdatePlayer(float deltaTime)
{
	MagneticCamera* camera = Manager::GetGameObject<MagneticCamera>();
	Vector3 movement{};
	if (camera != nullptr)
	{
		// 上下成分のないカメラ方向を使うので、視点を下げても移動方向は水平。
		const Vector3 forward = camera->GetFlatForward();
		const Vector3 right = camera->GetFlatRight();
		if (Input::GetKeyPress('W')) movement += forward;
		if (Input::GetKeyPress('S')) movement -= forward;
		if (Input::GetKeyPress('D')) movement += right;
		if (Input::GetKeyPress('A')) movement -= right;
	}
	const float movementLength = movement.length();
	// 斜め入力で速度が約1.41倍になるのを防ぐため、長さを1に揃える。
	if (movementLength > 1.0f)
	{
		movement /= movementLength;
	}

	// 目標速度へ徐々に近づける。入力を離した場合は目標が0となり減速する。
	const Vector3 desiredVelocity = movement * PlayerMoveSpeed;
	const float response = std::min(1.0f, 10.0f * deltaTime);
	m_PlayerVelocity.x += (desiredVelocity.x - m_PlayerVelocity.x) * response;
	m_PlayerVelocity.z += (desiredVelocity.z - m_PlayerVelocity.z) * response;
	// 接地中だけ、上向き速度を与えてジャンプさせる。
	if (Input::GetKeyTrigger(VK_SPACE) && m_PlayerGrounded)
	{
		m_PlayerVelocity.y = 8.5f;
		// 接地状態は、このあとの衝突処理で再判定する。
		m_PlayerGrounded = false;
	}

	// 加速度×時間で速度を更新し、速度×時間で位置を更新する。
	m_PlayerVelocity.y += Gravity * deltaTime;
	m_PlayerPosition += m_PlayerVelocity * deltaTime;
	// 接地状態は、このあとの衝突処理で再判定する。
	m_PlayerGrounded = false;
	ResolveStaticCollisions(
		m_PlayerPosition,
		m_PlayerVelocity,
		m_PlayerHalfExtent,
		&m_PlayerGrounded);

	// 落下またはHP切れで同じ階を初期状態からやり直す。
	if (m_PlayerPosition.y < -8.0f || m_Health <= 0)
	{
		BuildFloor(m_CurrentFloor);
	}
}

// 物体の種類で処理を振り分ける。各物体の更新前位置も保存する。
void MagneticPrototype::UpdateBodies(float deltaTime)
{
	for (Body& body : m_Bodies)
	{
		body.PreviousPosition = body.Position;
		switch (body.Kind)
		{
		case BodyKind::Crate:
			UpdateCrate(body, deltaTime);
			break;
		case BodyKind::Balloon:
			UpdateBalloon(body, deltaTime);
			break;
		case BodyKind::Enemy:
			UpdateEnemy(body, deltaTime);
			break;
		default:
			break;
		}
	}
}

// 箱を更新する。保持中は前方へ固定し、それ以外は磁力・重力・減速・風船との接触を処理する。
void MagneticPrototype::UpdateCrate(Body& body, float deltaTime)
{
	if (!body.Active)
	{
		return;
	}
	// 保持中の箱は物理移動せず、主人公の前方へ直接配置する。
	if (body.Held)
	{
		body.Position = m_PlayerPosition + GetViewForward() * 1.8f + Vector3(0.0f, 0.65f, 0.0f);
		body.Velocity = {};
		return;
	}

	Vector3 fromPlayer = body.Position - m_PlayerPosition;
	const float distance = fromPlayer.length();
	if (distance > 0.001f && distance < MagnetRadius)
	{
		fromPlayer /= distance;
		const float direction = -MagnetLogic::ForceDirection(m_PlayerPolarity, body.Polarity);
		const float force = MagnetLogic::ForceMagnitude(distance, MagnetRadius, 34.0f);
		body.Velocity += fromPlayer * (direction * force * deltaTime);
	}

	// 重力で速度を変え、その速度で物体を移動する。
	body.Velocity.y += Gravity * deltaTime;
	body.Position += body.Velocity * deltaTime;
	ResolveStaticCollisions(body.Position, body.Velocity, body.HalfExtent, nullptr);
	// 水平速度を少しずつ減らす簡易的な抵抗。
	body.Velocity.x *= std::max(0.0f, 1.0f - 0.7f * deltaTime);
	body.Velocity.z *= std::max(0.0f, 1.0f - 0.7f * deltaTime);

	for (Body& balloon : m_Bodies)
	{
		if (!balloon.Active || balloon.Kind != BodyKind::Balloon)
		{
			continue;
		}
		// 十分な速さの箱が風船に重なった場合だけ、風船を割る。
		if (body.Velocity.length() > 3.0f && Overlaps(body.Position, body.HalfExtent, balloon.Position, balloon.HalfExtent))
		{
			balloon.Active = false;
			balloon.RespawnRemaining = 5.0f;
			// 各風船の初回だけ加点する。復活後に再び割っても重複加点しない。
			if (!balloon.Counted)
			{
				balloon.Counted = true;
				++m_BalloonsPopped;
				m_Score += 100;
			}
			// 箱の速度を逆向きの25％にし、弱く跳ね返す。
			body.Velocity *= -0.25f;
		}
	}
}

// 割れた風船の復活待ち時間を減らす。時間切れで元の位置に復活するが、Countedは戻さない。
void MagneticPrototype::UpdateBalloon(Body& body, float deltaTime)
{
	if (body.Active)
	{
		return;
	}
	if (MagnetLogic::TickRespawn(body.RespawnRemaining, deltaTime))
	{
		body.Active = true;
		body.Position = body.SpawnPosition;
		body.Velocity = {};
	}
}

// 主人公への追跡加速と磁力を合成し、重力・速度上限・固定物との衝突・接触ダメージを処理する。
void MagneticPrototype::UpdateEnemy(Body& body, float deltaTime)
{
	if (!body.Active)
	{
		return;
	}
	Vector3 toPlayer = m_PlayerPosition - body.Position;
	const float distance = toPlayer.length();
	if (distance > 0.001f)
	{
		toPlayer /= distance;
		// 磁力とは別に、主人公へ向かう通常の追跡加速を加える。
		body.Velocity += toPlayer * (4.0f * deltaTime);
		const float magnetDirection = MagnetLogic::ForceDirection(m_PlayerPolarity, body.Polarity);
		const float magnetForce = MagnetLogic::ForceMagnitude(distance, MagnetRadius, 28.0f);
		body.Velocity += toPlayer * (magnetDirection * magnetForce * deltaTime);
	}
	// 重力で速度を変え、その速度で物体を移動する。
	body.Velocity.y += Gravity * deltaTime;
	const float speed = body.Velocity.length();
	// 速度ベクトル全体の長さを7以下に制限する（上下速度も含む）。
	if (speed > 7.0f)
	{
		body.Velocity *= 7.0f / speed;
	}
	body.Position += body.Velocity * deltaTime;
	ResolveStaticCollisions(body.Position, body.Velocity, body.HalfExtent, nullptr);

	// 無敵時間が終わっていて、敵と主人公が重なったときだけダメージ。
	if (m_DamageCooldown <= 0.0f && Overlaps(m_PlayerPosition, m_PlayerHalfExtent, body.Position, body.HalfExtent))
	{
		--m_Health;
		m_DamageCooldown = 1.0f;
		// 敵から離れる方向と上方向へ押し返す。
		m_PlayerVelocity += toPlayer * -8.0f + Vector3(0.0f, 4.0f, 0.0f);
	}
}

// 磁性の固定物から主人公への力を計算する。物体の中心ではなく、表面の最寄り点までの距離を使う。
void MagneticPrototype::ApplyMagneticSurfaceForces(float deltaTime)
{
	for (const StaticBlock& block : m_StaticBlocks)
	{
		if (!block.Magnetic)
		{
			continue;
		}
		// 主人公の各座標を箱の最小～最大座標へ収め、箱上の最寄り点を求める。
		const Vector3 closest{
			std::clamp(m_PlayerPosition.x, block.Position.x - block.HalfExtent.x, block.Position.x + block.HalfExtent.x),
			std::clamp(m_PlayerPosition.y, block.Position.y - block.HalfExtent.y, block.Position.y + block.HalfExtent.y),
			std::clamp(m_PlayerPosition.z, block.Position.z - block.HalfExtent.z, block.Position.z + block.HalfExtent.z),
		};
		Vector3 away = m_PlayerPosition - closest;
		const float distance = away.length();
		if (distance <= 0.001f || distance >= MagnetRadius)
		{
			continue;
		}
		away /= distance;
		const float direction = -MagnetLogic::ForceDirection(m_PlayerPolarity, block.Polarity);
		const float force = MagnetLogic::ForceMagnitude(distance, MagnetRadius, 32.0f);
		m_PlayerVelocity += away * (direction * force * deltaTime);
	}
}

// 3階限定の処理。部屋中央から水平距離8以内にいる間、1.2秒ごとに極性を反転する。
void MagneticPrototype::ApplyCoilInterference(float deltaTime)
{
	if (m_CurrentFloor != 3)
	{
		return;
	}
	Vector3 horizontal = m_PlayerPosition;
	// 高さを無視して、部屋中央からの水平距離だけを調べる。
	horizontal.y = 0.0f;
	if (horizontal.length() > 8.0f)
	{
		m_CoilTimer = 0.0f;
		return;
	}
	m_CoilTimer += deltaTime;
	if (m_CoilTimer >= 1.2f)
	{
		m_CoilTimer -= 1.2f;
		m_PlayerPolarity = MagnetLogic::Opposite(m_PlayerPolarity);
	}
}

// 各階の達成条件を調べてフラグを更新する。この関数自体はシーン切り替えを行わない。
void MagneticPrototype::CheckFloorGoal()
{
	switch (m_CurrentFloor)
	{
	case 1:
		m_FloorComplete = m_BalloonsPopped >= 5;
		break;
	case 2:
		m_FloorComplete = m_PlayerPosition.z >= 20.0f;
		break;
	case 3:
		m_FloorComplete = m_PlayerPosition.z >= 9.0f && m_Health > 0;
		break;
	case 4:
		m_FloorComplete = m_PlayerPosition.z >= 8.0f && m_PlayerPosition.y >= 5.8f;
		break;
	default:
		break;
	}
}

// 階・極性・HP・得点などを文字列へ変換し、変更があった場合だけウィンドウタイトルへ反映する。
void MagneticPrototype::UpdateCaption()
{
	const wchar_t* polarity = m_PlayerPolarity == MagnetLogic::Polarity::North ? L"N" : L"S";
	wchar_t caption[512]{};
	swprintf_s(
		caption,
		L"ZICKYY MAGNET BETA | Floor %d | Pole %s | HP %d | Score %d | Balloons %d/5%s | WASD Move  Space Jump  V Switch  C Grab  B Pulse/Throw  1-4 Floor  R Reset",
		m_CurrentFloor,
		polarity,
		m_Health,
		m_Score,
		m_BalloonsPopped,
		m_FloorComplete ? L" | CLEAR" : L"");
	if (m_LastCaption != caption)
	{
		m_LastCaption = caption;
		SetWindowTextW(GetWindow(), m_LastCaption.c_str());
	}
}

// 固定物とのAABB衝突を解決する。positionとvelocityは参照なので呼び出し元の値が変更される。
// groundedがnullptrでなければ、上向きに押し戻したとき接地フラグを立てる。動く箱同士などは対象外。
bool MagneticPrototype::ResolveStaticCollisions(
	Vector3& position,
	Vector3& velocity,
	const Vector3& halfExtent,
	bool* grounded) const
{
	bool collided = false;
	for (const StaticBlock& block : m_StaticBlocks)
	{
		// 中心間の距離を使い、各軸について「半寸法の合計−距離」でめり込み量を求める。
		const Vector3 delta = position - block.Position;
		const float overlapX = halfExtent.x + block.HalfExtent.x - std::abs(delta.x);
		const float overlapY = halfExtent.y + block.HalfExtent.y - std::abs(delta.y);
		const float overlapZ = halfExtent.z + block.HalfExtent.z - std::abs(delta.z);
		// どれか1軸でも重なっていなければ衝突していない。
		if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
		{
			continue;
		}

		collided = true;
		// 最小のめり込み方向へ押し出す。ここは上下方向が最も浅い場合。
		if (overlapY <= overlapX && overlapY <= overlapZ)
		{
			const float direction = delta.y >= 0.0f ? 1.0f : -1.0f;
			position.y += direction * overlapY;
			velocity.y = 0.0f;
			// 接地結果の格納先があり、上へ押し出した場合だけ接地扱いにする。
			if (grounded != nullptr && direction > 0.0f)
			{
				*grounded = true;
			}
		}
		else if (overlapX <= overlapZ)
		{
			const float direction = delta.x >= 0.0f ? 1.0f : -1.0f;
			position.x += direction * overlapX;
			velocity.x = 0.0f;
		}
		else
		{
			const float direction = delta.z >= 0.0f ? 1.0f : -1.0f;
			position.z += direction * overlapZ;
			velocity.z = 0.0f;
		}
	}
	return collided;
}

// 2つの直方体がX・Y・Zすべての軸で重なるかを調べる。位置は中心、寸法は半分の長さ。
bool MagneticPrototype::Overlaps(
	const Vector3& firstPosition,
	const Vector3& firstHalfExtent,
	const Vector3& secondPosition,
	const Vector3& secondHalfExtent) const
{
	return
		MagnetLogic::Overlaps(firstPosition.x, firstHalfExtent.x, secondPosition.x, secondHalfExtent.x) &&
		MagnetLogic::Overlaps(firstPosition.y, firstHalfExtent.y, secondPosition.y, secondHalfExtent.y) &&
		MagnetLogic::Overlaps(firstPosition.z, firstHalfExtent.z, secondPosition.z, secondHalfExtent.z);
}

// カメラの水平な前方向を返す。カメラがなければZプラス方向を使う。
Vector3 MagneticPrototype::GetViewForward() const
{
	MagneticCamera* camera = Manager::GetGameObject<MagneticCamera>();
	return camera != nullptr ? camera->GetFlatForward() : Vector3(0.0f, 0.0f, 1.0f);
}

// カメラの水平な右方向を返す。カメラがなければXプラス方向を使う。
Vector3 MagneticPrototype::GetViewRight() const
{
	MagneticCamera* camera = Manager::GetGameObject<MagneticCamera>();
	return camera != nullptr ? camera->GetFlatRight() : Vector3(1.0f, 0.0f, 0.0f);
}

// 有効・未保持・範囲内・前方寄りの箱から最も近いものを探す。戻り値−1は候補なし。
int MagneticPrototype::FindGrabbableBodyIndex() const
{
	float closestDistance = std::numeric_limits<float>::max();
	int closestIndex = -1;
	const Vector3 forward = GetViewForward();
	for (size_t index = 0; index < m_Bodies.size(); ++index)
	{
		const Body& body = m_Bodies[index];
		if (!body.Active || body.Held || body.Kind != BodyKind::Crate)
		{
			continue;
		}

		Vector3 toBody = body.Position - m_PlayerPosition;
		const float distance = toBody.length();
		if (distance <= 0.001f || distance > MagnetRadius)
		{
			continue;
		}
		toBody /= distance;
		// 内積で前方寄りかを確認する。さらに、現在の候補より近い箱だけを採用する。
		if (Vector3::dot(forward, toBody) < 0.15f || distance >= closestDistance)
		{
			continue;
		}

		closestDistance = distance;
		closestIndex = static_cast<int>(index);
	}
	return closestIndex;
}

// 現在の階に対応する説明文字列を返す。実際の達成判定はCheckFloorGoalが担当する。
const char* MagneticPrototype::GetFloorGoalText() const
{
	switch (m_CurrentFloor)
	{
	case 1:
		return "GOAL: POP 5 BALLOONS";
	case 2:
		return "GOAL: REACH THE EXIT";
	case 3:
		return "GOAL: CROSS THE COIL ROOM";
	case 4:
		return "GOAL: BUILD STAIRS TO EXIT";
	default:
		return "GOAL: EXPLORE";
	}
}

// 3D描画用の入力形式・シェーダー・頂点バッファを設定して、世界とHUDを順に描く。
void MagneticPrototype::Draw()
{
	Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
	Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
	// 頂点を3個ずつ組にして三角形として描画する。
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawWorld();
	DrawHud();
}

// 共有の立方体を拡大・移動して描く。元が−1～1の形なので拡大率にhalfExtentを使う。
void MagneticPrototype::DrawCube(
	const Vector3& position,
	const Vector3& halfExtent,
	const XMFLOAT4& color) const
{
	const XMMATRIX world =
		XMMatrixScaling(halfExtent.x, halfExtent.y, halfExtent.z) *
		XMMatrixTranslation(position.x, position.y, position.z);
	Renderer::SetWorldMatrix(world);

	MATERIAL material{};
	material.Ambient = color;
	material.Diffuse = color;
	material.Specular = { 0.18f, 0.18f, 0.18f, 1.0f };
	material.Emission = { 0.0f, 0.0f, 0.0f, 0.0f };
	material.Shininess = 12.0f;
	material.TextureEnable = false;
	Renderer::SetMaterial(material);
	// 共有バッファの先頭から36頂点（立方体1個）を描画する。
	Renderer::GetDeviceContext()->Draw(36, 0);
}

// 固定物、主人公、箱・風船・敵、取得候補の印、磁力範囲とコイルの見た目を描く。
void MagneticPrototype::DrawWorld() const
{
	for (const StaticBlock& block : m_StaticBlocks)
	{
		DrawCube(block.Position, block.HalfExtent, block.Color);
	}

	const XMFLOAT4 playerColor = PolarityColor(m_PlayerPolarity);
	DrawCube(m_PlayerPosition, m_PlayerHalfExtent, playerColor);
	DrawCube(m_PlayerPosition + Vector3(0.0f, 1.18f, 0.0f), { 0.34f, 0.34f, 0.34f }, { 0.95f, 0.92f, 0.78f, 1.0f });

	// 保持中でなければ、取得候補の箱を調べて表示色や目印に使う。
	const int grabbableBodyIndex = m_HeldBodyIndex >= 0 ? -1 : FindGrabbableBodyIndex();
	for (size_t bodyIndex = 0; bodyIndex < m_Bodies.size(); ++bodyIndex)
	{
		const Body& body = m_Bodies[bodyIndex];
		if (!body.Active)
		{
			continue;
		}
		switch (body.Kind)
		{
		case BodyKind::Crate:
			DrawCube(body.Position, body.HalfExtent, PolarityColor(body.Polarity));
			if (static_cast<int>(bodyIndex) == grabbableBodyIndex)
			{
				DrawCube(
					body.Position + Vector3(0.0f, body.HalfExtent.y + 0.35f, 0.0f),
					{ 0.28f, 0.08f, 0.28f },
					{ 1.0f, 0.82f, 0.16f, 1.0f });
			}
			else if (body.Held)
			{
				DrawCube(
					body.Position + Vector3(0.0f, body.HalfExtent.y + 0.35f, 0.0f),
					{ 0.28f, 0.08f, 0.28f },
					{ 0.25f, 1.0f, 0.42f, 1.0f });
			}
			break;
		case BodyKind::Balloon:
			DrawCube(body.Position, body.HalfExtent, { 1.0f, 0.48f, 0.72f, 1.0f });
			DrawCube(body.Position + Vector3(0.0f, -1.25f, 0.0f), { 0.08f, 0.28f, 0.08f }, { 0.9f, 0.9f, 0.9f, 1.0f });
			break;
		case BodyKind::Enemy:
			DrawCube(body.Position, body.HalfExtent, PolarityColor(body.Polarity));
			DrawCube(body.Position + Vector3(0.0f, 0.95f, 0.0f), { 0.2f, 0.35f, 0.2f }, { 0.85f, 0.2f, 1.0f, 1.0f });
			break;
		default:
			break;
		}
	}

	// 主人公を中心とする円周上に小さな立方体を並べ、磁力半径を可視化する。
	const XMFLOAT4 fieldColor = PolarityColor(m_PlayerPolarity);
	for (int index = 0; index < 12; ++index)
	{
		const float angle = static_cast<float>(index) / 12.0f * Pi * 2.0f;
		const Vector3 marker = m_PlayerPosition + Vector3(std::cos(angle) * MagnetRadius, 0.15f, std::sin(angle) * MagnetRadius);
		DrawCube(marker, { 0.07f, 0.07f, 0.07f }, fieldColor);
	}

	if (m_CurrentFloor == 3)
	{
		for (int index = 0; index < 20; ++index)
		{
			const float angle = static_cast<float>(index) / 20.0f * Pi * 2.0f;
			const Vector3 position{ std::cos(angle) * 3.0f, 1.2f, std::sin(angle) * 3.0f };
			DrawCube(position, { 0.22f, 1.2f, 0.22f }, index % 2 == 0 ? NorthColor() : SouthColor());
		}
	}
}

// HUDの四角形・ドット文字をCPU側でまとめ、GPUへ転送して一括描画する。
void MagneticPrototype::DrawHud() const
{
	// 今回のHUDを一から組み立てるため、前回分の頂点を空にする。
	m_HudVertices.clear();

	const XMFLOAT4 panel{ 0.025f, 0.045f, 0.075f, 0.86f };
	const XMFLOAT4 panelStrong{ 0.035f, 0.065f, 0.11f, 0.95f };
	const XMFLOAT4 white{ 0.96f, 0.98f, 1.0f, 1.0f };
	const XMFLOAT4 muted{ 0.72f, 0.80f, 0.88f, 1.0f };
	const XMFLOAT4 gold{ 1.0f, 0.78f, 0.16f, 1.0f };
	const XMFLOAT4 green{ 0.25f, 1.0f, 0.48f, 1.0f };
	const XMFLOAT4 red{ 1.0f, 0.12f, 0.14f, 0.72f };

	// 保持中でなければ、取得候補の箱を調べて表示色や目印に使う。
	const int grabbableBodyIndex = m_HeldBodyIndex >= 0 ? -1 : FindGrabbableBodyIndex();
	// 保持中は緑、取得候補ありは金色、それ以外は白で照準を描く。
	const XMFLOAT4 reticleColor =
		m_HeldBodyIndex >= 0 ? green : (grabbableBodyIndex >= 0 ? gold : white);
	const float centerX = static_cast<float>(SCREEN_WIDTH) * 0.5f;
	const float centerY = static_cast<float>(SCREEN_HEIGHT) * 0.5f;
	DrawHudRectangle(centerX - 13.0f, centerY - 1.5f, 26.0f, 3.0f, reticleColor);
	DrawHudRectangle(centerX - 1.5f, centerY - 13.0f, 3.0f, 26.0f, reticleColor);

	DrawHudRectangle(20.0f, 20.0f, 270.0f, 108.0f, panel);
	char floorText[32]{};
	sprintf_s(floorText, "FLOOR %d", m_CurrentFloor);
	DrawHudText(floorText, 38.0f, 36.0f, 2.4f, white);

	char healthText[32]{};
	sprintf_s(healthText, "HP %d", m_Health);
	DrawHudText(healthText, 38.0f, 72.0f, 2.0f, white);
	char scoreText[32]{};
	sprintf_s(scoreText, "SCORE %d", m_Score);
	DrawHudText(scoreText, 38.0f, 99.0f, 1.65f, muted);

	const XMFLOAT4 polarityColor = PolarityColor(m_PlayerPolarity);
	// 極性切り替え直後だけバッジを拡大する。
	const float badgeExpansion = m_PolarityFlashRemaining > 0.0f ? 5.0f : 0.0f;
	DrawHudRectangle(
		218.0f - badgeExpansion,
		38.0f - badgeExpansion,
		52.0f + badgeExpansion * 2.0f,
		52.0f + badgeExpansion * 2.0f,
		polarityColor);
	DrawHudText(
		m_PlayerPolarity == MagnetLogic::Polarity::North ? "N" : "S",
		244.0f,
		51.0f,
		3.7f,
		white,
		true);
	DrawHudText("POLE", 244.0f, 99.0f, 1.35f, muted, true);

	char goalText[64]{};
	if (m_CurrentFloor == 1)
	{
		sprintf_s(goalText, "GOAL: POP %d/5 BALLOONS", m_BalloonsPopped);
	}
	else
	{
		strcpy_s(goalText, GetFloorGoalText());
	}
	DrawHudRectangle(350.0f, 20.0f, 580.0f, 48.0f, panel);
	DrawHudText(goalText, centerX, 37.0f, 2.15f, white, true);

	DrawHudRectangle(1162.0f, 20.0f, 98.0f, 36.0f, panel);
	DrawHudText("H HELP", 1211.0f, 32.0f, 1.55f, muted, true);

	// 保持中／取得可能／それ以外で、画面下の操作案内を切り替える。
	const char* actionText = "B PULSE   V SWITCH POLE";
	XMFLOAT4 actionColor = white;
	if (m_HeldBodyIndex >= 0)
	{
		actionText = "B THROW   C DROP";
		actionColor = green;
	}
	else if (grabbableBodyIndex >= 0)
	{
		actionText = "C GRAB BOX";
		actionColor = gold;
	}
	DrawHudRectangle(330.0f, 648.0f, 620.0f, 50.0f, panelStrong);
	DrawHudText(actionText, centerX, 666.0f, 2.4f, actionColor, true);

	// ヘルプが開いている間は操作一覧を表示する。ゲーム更新自体は停止しない。
	if (m_HelpVisible)
	{
		DrawHudRectangle(315.0f, 142.0f, 650.0f, 430.0f, panelStrong);
		DrawHudText("CONTROLS", centerX, 170.0f, 3.3f, white, true);
		DrawHudText("WASD   MOVE", 390.0f, 225.0f, 2.3f, white);
		DrawHudText("MOUSE  LOOK", 390.0f, 265.0f, 2.3f, white);
		DrawHudText("SPACE  JUMP", 390.0f, 305.0f, 2.3f, white);
		DrawHudText("V      SWITCH POLE", 390.0f, 345.0f, 2.3f, white);
		DrawHudText("C      GRAB / DROP", 390.0f, 385.0f, 2.3f, white);
		DrawHudText("B      PULSE / THROW", 390.0f, 425.0f, 2.3f, white);
		DrawHudText("1-4    SELECT FLOOR", 390.0f, 465.0f, 2.3f, white);
		DrawHudText("R      RESET", 390.0f, 505.0f, 2.3f, white);
		DrawHudText("H      CLOSE HELP", 390.0f, 545.0f, 1.8f, muted);
	}
	// ヘルプを開いていない場合に限り、階開始直後の説明を表示する。
	else if (m_FloorIntroRemaining > 0.0f)
	{
		DrawHudRectangle(330.0f, 205.0f, 620.0f, 270.0f, panelStrong);
		DrawHudText(floorText, centerX, 238.0f, 4.0f, white, true);
		DrawHudText(GetFloorGoalText(), centerX, 300.0f, 2.3f, gold, true);

		const char* firstHint = "V SWITCH POLE";
		const char* secondHint = "USE MAGNET FORCE";
		switch (m_CurrentFloor)
		{
		case 1:
			firstHint = "C GRAB BOX";
			secondHint = "B THROW AT BALLOON";
			break;
		case 2:
			firstHint = "V SWITCH POLE";
			secondHint = "USE WALLS AND FLOOR";
			break;
		case 3:
			firstHint = "COIL SWITCHES YOUR POLE";
			secondHint = "MOVE TO THE EXIT";
			break;
		case 4:
			firstHint = "MOVE MAGNET BOXES";
			secondHint = "BUILD STAIRS TO EXIT";
			break;
		default:
			break;
		}
		DrawHudText(firstHint, centerX, 350.0f, 2.2f, white, true);
		DrawHudText(secondHint, centerX, 390.0f, 2.2f, white, true);
		DrawHudText("H HELP", centerX, 438.0f, 1.7f, muted, true);
	}

	if (m_FloorComplete)
	{
		DrawHudRectangle(335.0f, 225.0f, 610.0f, 225.0f, panelStrong);
		DrawHudText("FLOOR CLEAR", centerX, 264.0f, 4.0f, green, true);
		DrawHudText("ENTER  NEXT FLOOR", centerX, 337.0f, 2.6f, white, true);
		DrawHudText("R  RETRY", centerX, 395.0f, 1.8f, muted, true);
	}

	// ダメージ直後の短時間だけ、画面の四辺に赤い枠を重ねる。
	if (m_DamageCooldown > 0.65f)
	{
		DrawHudRectangle(0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), 14.0f, red);
		DrawHudRectangle(0.0f, static_cast<float>(SCREEN_HEIGHT) - 14.0f, static_cast<float>(SCREEN_WIDTH), 14.0f, red);
		DrawHudRectangle(0.0f, 0.0f, 14.0f, static_cast<float>(SCREEN_HEIGHT), red);
		DrawHudRectangle(static_cast<float>(SCREEN_WIDTH) - 14.0f, 0.0f, 14.0f, static_cast<float>(SCREEN_HEIGHT), red);
	}

	if (m_HudVertices.empty())
	{
		return;
	}

	// HUDが3D物体の後ろに隠れないよう、奥行き判定を無効にする。
	Renderer::SetDepthEnable(false);
	// 画面のピクセル座標で配置できる2D用の行列へ切り替える。
	Renderer::SetWorldViewProjection2D();

	// GPUバッファの書き込み先をMapで取得する。WRITE_DISCARDで内容を更新し直す。
	D3D11_MAPPED_SUBRESOURCE mapped{};
	ThrowIfFailed(
		Renderer::GetDeviceContext()->Map(
			m_HudVertexBuffer,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mapped),
		"Map magnetic HUD vertex buffer");
	// 組み立てたHUD頂点をGPU用バッファへコピーし、Unmapで書き込みを終える。
	std::memcpy(
		mapped.pData,
		m_HudVertices.data(),
		m_HudVertices.size() * sizeof(VERTEX_3D));
	Renderer::GetDeviceContext()->Unmap(m_HudVertexBuffer, 0);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_HudVertexBuffer, &stride, &offset);
	Renderer::GetDeviceContext()->PSSetShader(m_HudPixelShader, nullptr, 0);
	Renderer::GetDeviceContext()->Draw(static_cast<UINT>(m_HudVertices.size()), 0);
	// 後続の描画に備えて、奥行き判定を有効へ戻す。
	Renderer::SetDepthEnable(true);
}

// 画面座標の四角形を2三角形＝6頂点にしてHUD一覧へ追加する。ここではまだGPU描画しない。
void MagneticPrototype::DrawHudRectangle(
	float left,
	float top,
	float width,
	float height,
	const XMFLOAT4& color) const
{
	// 無効な大きさやバッファ上限を超える四角形は追加しない。
	if (width <= 0.0f || height <= 0.0f || m_HudVertices.size() + 6 > MaxHudVertices)
	{
		return;
	}

	const float right = left + width;
	const float bottom = top + height;
	const auto vertex = [&color](float x, float y)
		{
			VERTEX_3D result{};
			result.Position = { x, y, 0.1f };
			result.Normal = { 0.0f, 0.0f, -1.0f };
			result.Diffuse = color;
			result.TexCoord = { 0.0f, 0.0f };
			return result;
		};

	m_HudVertices.push_back(vertex(left, top));
	m_HudVertices.push_back(vertex(right, top));
	m_HudVertices.push_back(vertex(right, bottom));
	m_HudVertices.push_back(vertex(left, top));
	m_HudVertices.push_back(vertex(right, bottom));
	m_HudVertices.push_back(vertex(left, bottom));
}

// 5列×7行の文字パターンを読み、立っているビットを小さな四角形としてHUD一覧へ追加する。
void MagneticPrototype::DrawHudText(
	const char* text,
	float x,
	float y,
	float scale,
	const XMFLOAT4& color,
	bool centered) const
{
	if (text == nullptr || scale <= 0.0f)
	{
		return;
	}
	// 指定X座標が文字列の中央になるよう、開始位置を半分の幅だけ左へずらす。
	if (centered)
	{
		x -= MeasureHudText(text, scale) * 0.5f;
	}

	float cursorX = x;
	for (const char* character = text; *character != '\0'; ++character)
	{
		const std::array<unsigned char, 7> glyph = HudGlyph(*character);
		for (int row = 0; row < 7; ++row)
		{
			for (int column = 0; column < 5; ++column)
			{
				// 左端は4ビット目、右端は0ビット目。該当ビットが0ならその点を描かない。
				if ((glyph[static_cast<size_t>(row)] & (1u << (4 - column))) == 0)
				{
					continue;
				}
				DrawHudRectangle(
					cursorX + static_cast<float>(column) * scale,
					y + static_cast<float>(row) * scale,
					// 点をセルの84％の幅・高さにして、点と点の間に隙間を作る。
					scale * 0.84f,
					// 点をセルの84％の幅・高さにして、点と点の間に隙間を作る。
					scale * 0.84f,
					color);
			}
		}
		// 5列の文字幅に1列の余白を足して、次の文字へ進む。
		cursorX += scale * 6.0f;
	}
}

// 文字列の表示幅を計算する。1文字の送り幅は6×scaleで、末尾の余白1×scaleを引く。
float MagneticPrototype::MeasureHudText(const char* text, float scale) const
{
	if (text == nullptr || *text == '\0' || scale <= 0.0f)
	{
		return 0.0f;
	}
	return (static_cast<float>(std::strlen(text)) * 6.0f - 1.0f) * scale;
}
