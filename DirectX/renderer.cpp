
#include "main.h"
#include "renderer.h"
#include <fstream>


D3D_FEATURE_LEVEL       Renderer::m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device* Renderer::m_Device{};
ID3D11DeviceContext* Renderer::m_DeviceContext{};
IDXGISwapChain* Renderer::m_SwapChain{};
ID3D11RenderTargetView* Renderer::m_RenderTargetView{};
ID3D11DepthStencilView* Renderer::m_DepthStencilView{};

ID3D11Buffer* Renderer::m_WorldBuffer{};
ID3D11Buffer* Renderer::m_ViewBuffer{};
ID3D11Buffer* Renderer::m_ProjectionBuffer{};
ID3D11Buffer* Renderer::m_MaterialBuffer{};
ID3D11Buffer* Renderer::m_LightBuffer{};


ID3D11DepthStencilState* Renderer::m_DepthStateEnable{};
ID3D11DepthStencilState* Renderer::m_DepthStateDisable{};


ID3D11BlendState* Renderer::m_BlendState{};
ID3D11BlendState* Renderer::m_BlendStateAdd{};
ID3D11BlendState* Renderer::m_BlendStateATC{};




void Renderer::Init()
{
	HRESULT hr = S_OK;




	// デバイス、スワップチェーン作成
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = SCREEN_WIDTH;
	swapChainDesc.BufferDesc.Height = SCREEN_HEIGHT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = GetWindow();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&m_SwapChain,
		&m_Device,
		&m_FeatureLevel,
		&m_DeviceContext);
	ThrowIfFailed(hr, "D3D11CreateDeviceAndSwapChain");






	// レンダーターゲットビュー作成
	ID3D11Texture2D* renderTarget{};
	ThrowIfFailed(m_SwapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&renderTarget)),
		"IDXGISwapChain::GetBuffer");
	const HRESULT renderTargetResult =
		m_Device->CreateRenderTargetView(renderTarget, NULL, &m_RenderTargetView);
	SafeRelease(renderTarget);
	ThrowIfFailed(renderTargetResult, "ID3D11Device::CreateRenderTargetView");


	// デプスステンシルバッファ作成
	ID3D11Texture2D* depthStencile{};
	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = swapChainDesc.BufferDesc.Width;
	textureDesc.Height = swapChainDesc.BufferDesc.Height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_D16_UNORM;
	textureDesc.SampleDesc = swapChainDesc.SampleDesc;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	ThrowIfFailed(m_Device->CreateTexture2D(&textureDesc, NULL, &depthStencile),
		"ID3D11Device::CreateTexture2D(depth)");

	// デプスステンシルビュー作成
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = textureDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = 0;
	const HRESULT depthViewResult =
		m_Device->CreateDepthStencilView(depthStencile, &depthStencilViewDesc, &m_DepthStencilView);
	SafeRelease(depthStencile);
	ThrowIfFailed(depthViewResult, "ID3D11Device::CreateDepthStencilView");


	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);





	// ビューポート設定
	D3D11_VIEWPORT viewport;
	viewport.Width = (FLOAT)SCREEN_WIDTH;
	viewport.Height = (FLOAT)SCREEN_HEIGHT;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	m_DeviceContext->RSSetViewports(1, &viewport);



	// ラスタライザステート設定
	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;

	ID3D11RasterizerState* rs{};
	ThrowIfFailed(m_Device->CreateRasterizerState(&rasterizerDesc, &rs),
		"ID3D11Device::CreateRasterizerState");

	m_DeviceContext->RSSetState(rs);
	SafeRelease(rs);




	// ブレンドステート設定
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ThrowIfFailed(m_Device->CreateBlendState(&blendDesc, &m_BlendState),
		"ID3D11Device::CreateBlendState(alpha)");

	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	ThrowIfFailed(m_Device->CreateBlendState(&blendDesc, &m_BlendStateAdd),
		"ID3D11Device::CreateBlendState(add)");
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

	blendDesc.AlphaToCoverageEnable = TRUE;
	ThrowIfFailed(m_Device->CreateBlendState(&blendDesc, &m_BlendStateATC),
		"ID3D11Device::CreateBlendState(alpha-to-coverage)");

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);





	// デプスステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthStencilDesc.StencilEnable = FALSE;

	ThrowIfFailed(m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStateEnable),
		"ID3D11Device::CreateDepthStencilState(enable)");

	//depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	ThrowIfFailed(m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStateDisable),
		"ID3D11Device::CreateDepthStencilState(disable)");

	m_DeviceContext->OMSetDepthStencilState(m_DepthStateEnable, NULL);




	// サンプラーステート設定
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* samplerState{};
	ThrowIfFailed(m_Device->CreateSamplerState(&samplerDesc, &samplerState),
		"ID3D11Device::CreateSamplerState");

	m_DeviceContext->PSSetSamplers(0, 1, &samplerState);
	SafeRelease(samplerState);



	// 定数バッファ生成
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = sizeof(float);

	ThrowIfFailed(m_Device->CreateBuffer(&bufferDesc, NULL, &m_WorldBuffer),
		"ID3D11Device::CreateBuffer(world)");
	m_DeviceContext->VSSetConstantBuffers(0, 1, &m_WorldBuffer);

	ThrowIfFailed(m_Device->CreateBuffer(&bufferDesc, NULL, &m_ViewBuffer),
		"ID3D11Device::CreateBuffer(view)");
	m_DeviceContext->VSSetConstantBuffers(1, 1, &m_ViewBuffer);

	ThrowIfFailed(m_Device->CreateBuffer(&bufferDesc, NULL, &m_ProjectionBuffer),
		"ID3D11Device::CreateBuffer(projection)");
	m_DeviceContext->VSSetConstantBuffers(2, 1, &m_ProjectionBuffer);


	bufferDesc.ByteWidth = sizeof(MATERIAL);

	ThrowIfFailed(m_Device->CreateBuffer(&bufferDesc, NULL, &m_MaterialBuffer),
		"ID3D11Device::CreateBuffer(material)");
	m_DeviceContext->VSSetConstantBuffers(3, 1, &m_MaterialBuffer);
	m_DeviceContext->PSSetConstantBuffers(3, 1, &m_MaterialBuffer);


	bufferDesc.ByteWidth = sizeof(LIGHT);

	ThrowIfFailed(m_Device->CreateBuffer(&bufferDesc, NULL, &m_LightBuffer),
		"ID3D11Device::CreateBuffer(light)");
	m_DeviceContext->VSSetConstantBuffers(4, 1, &m_LightBuffer);
	m_DeviceContext->PSSetConstantBuffers(4, 1, &m_LightBuffer);





	// ライト初期化
	LIGHT light{};
	light.Enable = true;
	light.Direction = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	light.Diffuse = XMFLOAT4(1.5f, 1.5f, 1.5f, 1.0f);
	SetLight(light);



	// マテリアル初期化
	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	SetMaterial(material);




}



void Renderer::Uninit()
{
	if (m_DeviceContext != nullptr)
	{
		m_DeviceContext->ClearState();
	}

	SafeRelease(m_WorldBuffer);
	SafeRelease(m_ViewBuffer);
	SafeRelease(m_ProjectionBuffer);
	SafeRelease(m_LightBuffer);
	SafeRelease(m_MaterialBuffer);

	SafeRelease(m_DepthStateEnable);
	SafeRelease(m_DepthStateDisable);
	SafeRelease(m_BlendState);
	SafeRelease(m_BlendStateAdd);
	SafeRelease(m_BlendStateATC);

	SafeRelease(m_DepthStencilView);
	SafeRelease(m_RenderTargetView);
	SafeRelease(m_SwapChain);
	SafeRelease(m_DeviceContext);
	SafeRelease(m_Device);

}




void Renderer::Begin()
{
	SetDepthEnable(true);
	SetAddBlendEnable(false);
	//float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };	//クリア色(黒)
	float clearColor[4] = { 0.5f, 0.2f, 0.5f, 1.0f };	//クリア色(黒)
	m_DeviceContext->ClearRenderTargetView(m_RenderTargetView, clearColor);
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}



void Renderer::End()
{
	ThrowIfFailed(m_SwapChain->Present(1, 0), "IDXGISwapChain::Present");
}




void Renderer::SetDepthEnable(bool Enable)
{
	if (Enable)
		m_DeviceContext->OMSetDepthStencilState(m_DepthStateEnable, NULL);
	else
		m_DeviceContext->OMSetDepthStencilState(m_DepthStateDisable, NULL);

}

void Renderer::SetAddBlendEnable(bool Enable)
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
		m_DeviceContext->OMSetBlendState(m_BlendStateAdd, blendFactor, 0xffffffff);
	else
		m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);

}


void Renderer::SetATCEnable(bool Enable)
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
		m_DeviceContext->OMSetBlendState(m_BlendStateATC, blendFactor, 0xffffffff);
	else
		m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);

}

void Renderer::SetWorldViewProjection2D()
{
	SetWorldMatrix(XMMatrixIdentity());
	SetViewMatrix(XMMatrixIdentity());

	XMMATRIX projection;
	projection = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);
	SetProjectionMatrix(projection);
}


void Renderer::SetWorldMatrix(XMMATRIX WorldMatrix)
{
	XMFLOAT4X4 worldf;
	XMStoreFloat4x4(&worldf, XMMatrixTranspose(WorldMatrix));
	m_DeviceContext->UpdateSubresource(m_WorldBuffer, 0, NULL, &worldf, 0, 0);
}

void Renderer::SetViewMatrix(XMMATRIX ViewMatrix)
{
	XMFLOAT4X4 viewf;
	XMStoreFloat4x4(&viewf, XMMatrixTranspose(ViewMatrix));
	m_DeviceContext->UpdateSubresource(m_ViewBuffer, 0, NULL, &viewf, 0, 0);
}

void Renderer::SetProjectionMatrix(XMMATRIX ProjectionMatrix)
{
	XMFLOAT4X4 projectionf;
	XMStoreFloat4x4(&projectionf, XMMatrixTranspose(ProjectionMatrix));
	m_DeviceContext->UpdateSubresource(m_ProjectionBuffer, 0, NULL, &projectionf, 0, 0);

}



void Renderer::SetMaterial(MATERIAL Material)
{
	m_DeviceContext->UpdateSubresource(m_MaterialBuffer, 0, NULL, &Material, 0, 0);
}

void Renderer::SetLight(LIGHT Light)
{
	m_DeviceContext->UpdateSubresource(m_LightBuffer, 0, NULL, &Light, 0, 0);
}





void Renderer::CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName)
{
	std::ifstream file(FileName, std::ios::binary | std::ios::ate);
	if (!file)
	{
		throw std::runtime_error(std::string("Vertex shader file not found: ") + FileName);
	}

	const std::streamsize fileSize = file.tellg();
	if (fileSize <= 0)
	{
		throw std::runtime_error(std::string("Vertex shader file is empty: ") + FileName);
	}

	std::vector<unsigned char> buffer(static_cast<size_t>(fileSize));
	file.seekg(0, std::ios::beg);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
	{
		throw std::runtime_error(std::string("Failed to read vertex shader: ") + FileName);
	}

	ThrowIfFailed(m_Device->CreateVertexShader(buffer.data(), buffer.size(), NULL, VertexShader),
		"ID3D11Device::CreateVertexShader");


	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 10, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	ThrowIfFailed(m_Device->CreateInputLayout(layout,
		numElements,
		buffer.data(),
		buffer.size(),
		VertexLayout),
		"ID3D11Device::CreateInputLayout");
}



void Renderer::CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName)
{
	std::ifstream file(FileName, std::ios::binary | std::ios::ate);
	if (!file)
	{
		throw std::runtime_error(std::string("Pixel shader file not found: ") + FileName);
	}

	const std::streamsize fileSize = file.tellg();
	if (fileSize <= 0)
	{
		throw std::runtime_error(std::string("Pixel shader file is empty: ") + FileName);
	}

	std::vector<unsigned char> buffer(static_cast<size_t>(fileSize));
	file.seekg(0, std::ios::beg);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
	{
		throw std::runtime_error(std::string("Failed to read pixel shader: ") + FileName);
	}

	ThrowIfFailed(m_Device->CreatePixelShader(buffer.data(), buffer.size(), NULL, PixelShader),
		"ID3D11Device::CreatePixelShader");
}

void Renderer::CreateTextureFromFile(ID3D11ShaderResourceView** Texture, const wchar_t* FileName)
{
	TexMetadata metadata{};
	ScratchImage image;
	ThrowIfFailed(LoadFromWICFile(FileName, WIC_FLAGS_NONE, &metadata, image),
		"LoadFromWICFile");
	ThrowIfFailed(CreateShaderResourceView(
		m_Device,
		image.GetImages(),
		image.GetImageCount(),
		metadata,
		Texture),
		"CreateShaderResourceView");
}


