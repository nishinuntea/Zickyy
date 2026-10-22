
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "main.h"
#include "renderer.h"
#include "modelRenderer.h"
#include <cerrno>
#include <climits>
#include <vector>



namespace
{
	struct ObjVertexIndex
	{
		int Position{};
		int TexCoord{};
		int Normal{};
	};

	std::string GetDirectory(const std::string& path)
	{
		const size_t separator = path.find_last_of("\\/");
		return separator == std::string::npos ? std::string{} : path.substr(0, separator);
	}

	std::string JoinPath(const std::string& directory, const std::string& fileName)
	{
		if (directory.empty())
		{
			return fileName;
		}
		return directory + "\\" + fileName;
	}

	template <size_t Size>
	void CopyToFixedBuffer(char (&destination)[Size], const std::string& source, const char* fieldName)
	{
		if (source.size() >= Size)
		{
			throw std::runtime_error(std::string(fieldName) + " is too long");
		}
		memcpy(destination, source.c_str(), source.size() + 1);
	}

	template <size_t Size>
	bool ReadToken(FILE* file, char (&buffer)[Size])
	{
		static_assert(Size == 256, "Update the fscanf_s width when changing token buffer size");
		return fscanf_s(
			file,
			"%255s",
			buffer,
			static_cast<unsigned int>(Size)) == 1;
	}

	template <size_t Size, size_t ExpectedSize>
	bool TokenEquals(const char (&token)[Size], const char (&expected)[ExpectedSize])
	{
		return strncmp(token, expected, Size) == 0;
	}

	int ParseObjInteger(const std::string& value, const char* fieldName)
	{
		if (value.empty())
		{
			return 0;
		}

		errno = 0;
		char* end{};
		const long parsed = strtol(value.c_str(), &end, 10);
		if (errno == ERANGE || end == value.c_str() || *end != '\0' ||
			parsed < INT_MIN || parsed > INT_MAX)
		{
			throw std::runtime_error(std::string("Invalid OBJ ") + fieldName + " index");
		}
		return static_cast<int>(parsed);
	}

	ObjVertexIndex ParseObjVertexIndex(const std::string& token)
	{
		ObjVertexIndex result{};
		const size_t firstSlash = token.find('/');
		if (firstSlash == std::string::npos)
		{
			result.Position = ParseObjInteger(token, "position");
			return result;
		}

		result.Position = ParseObjInteger(token.substr(0, firstSlash), "position");
		const size_t secondSlash = token.find('/', firstSlash + 1);
		if (secondSlash == std::string::npos)
		{
			result.TexCoord = ParseObjInteger(token.substr(firstSlash + 1), "texture");
			return result;
		}
		if (token.find('/', secondSlash + 1) != std::string::npos)
		{
			throw std::runtime_error("Invalid OBJ face token");
		}

		result.TexCoord = ParseObjInteger(
			token.substr(firstSlash + 1, secondSlash - firstSlash - 1),
			"texture");
		result.Normal = ParseObjInteger(token.substr(secondSlash + 1), "normal");
		return result;
	}

	size_t ResolveObjIndex(int index, size_t elementCount, const char* fieldName)
	{
		if (index == 0)
		{
			throw std::runtime_error(std::string("OBJ face is missing ") + fieldName + " index");
		}

		const long long resolved = index > 0
			? static_cast<long long>(index) - 1
			: static_cast<long long>(elementCount) + index;
		if (resolved < 0 || resolved >= static_cast<long long>(elementCount))
		{
			throw std::runtime_error(std::string("OBJ ") + fieldName + " index is out of range");
		}
		return static_cast<size_t>(resolved);
	}

	UINT ToBufferByteWidth(size_t elementSize, unsigned int elementCount, const char* fieldName)
	{
		if (elementCount == 0 || elementSize > UINT_MAX / elementCount)
		{
			throw std::runtime_error(std::string(fieldName) + " buffer size exceeds Direct3D limits");
		}
		return static_cast<UINT>(elementSize * elementCount);
	}
}

std::unordered_map<std::string, MODEL*> ModelRenderer::m_ModelPool;


void ModelRenderer::Draw()
{

	// 頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_Model->VertexBuffer, &stride, &offset);

	// インデックスバッファ設定
	Renderer::GetDeviceContext()->IASetIndexBuffer(m_Model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// プリミティブトポロジ設定
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	for( unsigned int i = 0; i < m_Model->SubsetNum; i++ )
	{
		if (m_Flash)
		{
			// マテリアル設定
			MATERIAL material{};
			material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
			material.TextureEnable = false;
			Renderer::SetMaterial(material);

		}
		else
		{
			// マテリアル設定
			Renderer::SetMaterial(m_Model->SubsetArray[i].Material.Material);

			// テクスチャ設定
			if (m_Model->SubsetArray[i].Material.Texture)
				Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Model->SubsetArray[i].Material.Texture);

		}

		// ポリゴン描画
		Renderer::GetDeviceContext()->DrawIndexed(m_Model->SubsetArray[i].IndexNum, m_Model->SubsetArray[i].StartIndex, 0 );
	}

}

void ModelRenderer::Preload(const char *FileName)
{
	if (m_ModelPool.count(FileName) > 0)
	{
		return;
	}

	std::unique_ptr<MODEL> model = std::make_unique<MODEL>();
	LoadModel(FileName, model.get());

	m_ModelPool[FileName] = model.release();

}


void ModelRenderer::UnloadAll()
{
	for (std::pair<const std::string, MODEL*> pair : m_ModelPool)
	{
		SafeRelease(pair.second->VertexBuffer);
		SafeRelease(pair.second->IndexBuffer);

		for (unsigned int i = 0; i < pair.second->SubsetNum; i++)
		{
			if (pair.second->SubsetArray[i].Material.Texture)
				SafeRelease(pair.second->SubsetArray[i].Material.Texture);
		}

		delete[] pair.second->SubsetArray;

		delete pair.second;
	}

	m_ModelPool.clear();
}


void ModelRenderer::Load(const char *FileName)
{
	if (m_ModelPool.count(FileName) > 0)
	{
		m_Model = m_ModelPool[FileName];
		return;
	}

	std::unique_ptr<MODEL> model = std::make_unique<MODEL>();
	LoadModel(FileName, model.get());

	m_Model = model.release();
	m_ModelPool[FileName] = m_Model;

}

void ModelRenderer::LoadModel( const char *FileName, MODEL *Model)
{
	if (FileName == nullptr || Model == nullptr)
	{
		throw std::invalid_argument("ModelRenderer::LoadModel received a null argument");
	}

	MODEL_OBJ modelObj{};
	try
	{
		LoadObj(FileName, &modelObj);

		D3D11_BUFFER_DESC vertexBufferDesc{};
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = ToBufferByteWidth(
			sizeof(VERTEX_3D),
			modelObj.VertexNum,
			"Model vertex");
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexData{};
		vertexData.pSysMem = modelObj.VertexArray;
		ThrowIfFailed(Renderer::GetDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexData,
			&Model->VertexBuffer),
			"ID3D11Device::CreateBuffer(model vertices)");

		D3D11_BUFFER_DESC indexBufferDesc{};
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = ToBufferByteWidth(
			sizeof(unsigned int),
			modelObj.IndexNum,
			"Model index");
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA indexData{};
		indexData.pSysMem = modelObj.IndexArray;
		ThrowIfFailed(Renderer::GetDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexData,
			&Model->IndexBuffer),
			"ID3D11Device::CreateBuffer(model indices)");

		Model->SubsetArray = new SUBSET[modelObj.SubsetNum]{};
		Model->SubsetNum = modelObj.SubsetNum;

		for (unsigned int i = 0; i < modelObj.SubsetNum; i++)
		{
			Model->SubsetArray[i] = modelObj.SubsetArray[i];
			Model->SubsetArray[i].Material.Texture = nullptr;

			const char* textureName = modelObj.SubsetArray[i].Material.TextureName;
			if (textureName[0] != '\0')
			{
				const int wideLength = MultiByteToWideChar(
					CP_ACP,
					MB_ERR_INVALID_CHARS,
					textureName,
					-1,
					nullptr,
					0);
				if (wideLength <= 0)
				{
					throw std::runtime_error("Model texture path has invalid encoding");
				}

				std::vector<wchar_t> wideName(static_cast<size_t>(wideLength));
				if (MultiByteToWideChar(
					CP_ACP,
					MB_ERR_INVALID_CHARS,
					textureName,
					-1,
					wideName.data(),
					wideLength) == 0)
				{
					throw std::runtime_error("Failed to convert model texture path");
				}

				Renderer::CreateTextureFromFile(
					&Model->SubsetArray[i].Material.Texture,
					wideName.data());
			}

			Model->SubsetArray[i].Material.Material.TextureEnable =
				Model->SubsetArray[i].Material.Texture != nullptr;
		}
	}
	catch (...)
	{
		for (unsigned int i = 0; i < Model->SubsetNum; ++i)
		{
			SafeRelease(Model->SubsetArray[i].Material.Texture);
		}
		delete[] Model->SubsetArray;
		Model->SubsetArray = nullptr;
		Model->SubsetNum = 0;
		SafeRelease(Model->VertexBuffer);
		SafeRelease(Model->IndexBuffer);

		delete[] modelObj.VertexArray;
		delete[] modelObj.IndexArray;
		delete[] modelObj.SubsetArray;
		throw;
	}

	delete[] modelObj.VertexArray;
	delete[] modelObj.IndexArray;
	delete[] modelObj.SubsetArray;
}






//モデル読込////////////////////////////////////////////
void ModelRenderer::LoadObj( const char *FileName, MODEL_OBJ *ModelObj )
{
	if (FileName == nullptr || ModelObj == nullptr)
	{
		throw std::invalid_argument("ModelRenderer::LoadObj received a null argument");
	}

	const std::string directory = GetDirectory(FileName);





	XMFLOAT3	*positionArray{};
	XMFLOAT3	*normalArray{};
	XMFLOAT2	*texcoordArray{};

	unsigned int	positionNum = 0;
	unsigned int	normalNum = 0;
	unsigned int	texcoordNum = 0;
	unsigned int	vertexNum = 0;
	unsigned int	indexNum = 0;
	unsigned int	in = 0;
	unsigned int	subsetNum = 0;

	MODEL_MATERIAL	*materialArray = nullptr;
	std::unique_ptr<MODEL_MATERIAL[]> materialOwner;
	unsigned int	materialNum = 0;

	char str[256];
	int c{};


	FILE *file{};
	if (fopen_s(&file, FileName, "rt") != 0 || file == nullptr)
	{
		throw std::runtime_error(std::string("OBJ file not found: ") + FileName);
	}
	std::unique_ptr<FILE, decltype(&fclose)> fileHandle(file, &fclose);



	//要素数カウント
	while (ReadToken(file, str))
	{
		if (TokenEquals(str, "v"))
		{
			if (positionNum == UINT_MAX)
			{
				throw std::runtime_error("OBJ position count exceeds supported limits");
			}
			positionNum++;
		}
		else if (TokenEquals(str, "vn"))
		{
			if (normalNum == UINT_MAX)
			{
				throw std::runtime_error("OBJ normal count exceeds supported limits");
			}
			normalNum++;
		}
		else if (TokenEquals(str, "vt"))
		{
			if (texcoordNum == UINT_MAX)
			{
				throw std::runtime_error("OBJ texture coordinate count exceeds supported limits");
			}
			texcoordNum++;
		}
		else if (TokenEquals(str, "usemtl"))
		{
			if (subsetNum == UINT_MAX)
			{
				throw std::runtime_error("OBJ subset count exceeds supported limits");
			}
			subsetNum++;
		}
		else if (TokenEquals(str, "f"))
		{
			in = 0;

			do
			{
				if (!ReadToken(file, str))
				{
					throw std::runtime_error("Unexpected end of OBJ face");
				}
				if (vertexNum == UINT_MAX || in == UINT_MAX)
				{
					throw std::runtime_error("OBJ face count exceeds supported limits");
				}
				vertexNum++;
				in++;
				c = fgetc( file );
			}
			while( c != '\n' && c != '\r' && c != EOF );

			//四角は三角に分割
			if( in == 4 )
				in = 6;

			if (indexNum > UINT_MAX - in)
			{
				throw std::runtime_error("OBJ index count exceeds supported limits");
			}
			indexNum += in;
		}
	}

	if (positionNum == 0 || vertexNum == 0 || indexNum == 0)
	{
		throw std::runtime_error("OBJ file contains no renderable geometry");
	}
	if (subsetNum == 0)
	{
		subsetNum = 1;
	}


	//メモリ確保
	positionArray = new XMFLOAT3[positionNum]{};
	normalArray = new XMFLOAT3[normalNum]{};
	texcoordArray = new XMFLOAT2[texcoordNum]{};


	ModelObj->VertexArray = new VERTEX_3D[vertexNum]{};
	ModelObj->VertexNum = vertexNum;

	ModelObj->IndexArray = new unsigned int[indexNum]{};
	ModelObj->IndexNum = indexNum;

	ModelObj->SubsetArray = new SUBSET[subsetNum]{};
	ModelObj->SubsetNum = subsetNum;




	//要素読込
	XMFLOAT3 *position = positionArray;
	XMFLOAT3 *normal = normalArray;
	XMFLOAT2 *texcoord = texcoordArray;

	unsigned int vc = 0;
	unsigned int ic = 0;
	unsigned int sc = 0;


	fseek( file, 0, SEEK_SET );

	while (ReadToken(file, str))
	{
		if (TokenEquals(str, "mtllib"))
		{
			//マテリアルファイル
			if (!ReadToken(file, str))
			{
				throw std::runtime_error("OBJ material file name is missing");
			}
			const std::string path = JoinPath(directory, str);
			LoadMaterial(path.c_str(), &materialArray, &materialNum);
			materialOwner.reset(materialArray);
		}
		else if (TokenEquals(str, "o"))
		{
			//オブジェクト名
			if (!ReadToken(file, str))
			{
				throw std::runtime_error("OBJ object name is missing");
			}
		}
		else if (TokenEquals(str, "v"))
		{
			//頂点座標
			if (fscanf(file, "%f%f%f", &position->x, &position->y, &position->z) != 3)
			{
				throw std::runtime_error("Invalid OBJ position");
			}
			position++;
		}
		else if (TokenEquals(str, "vn"))
		{
			//法線
			if (fscanf(file, "%f%f%f", &normal->x, &normal->y, &normal->z) != 3)
			{
				throw std::runtime_error("Invalid OBJ normal");
			}
			normal++;
		}
		else if (TokenEquals(str, "vt"))
		{
			//テクスチャ座標
			if (fscanf(file, "%f%f", &texcoord->x, &texcoord->y) != 2)
			{
				throw std::runtime_error("Invalid OBJ texture coordinate");
			}
			texcoord->x = 1.0f - texcoord->x;
			texcoord->y = 1.0f - texcoord->y;
			texcoord++;
		}
		else if (TokenEquals(str, "usemtl"))
		{
			//マテリアル
			if (!ReadToken(file, str) || sc >= subsetNum)
			{
				throw std::runtime_error("Invalid OBJ material subset");
			}

			if( sc != 0 )
				ModelObj->SubsetArray[ sc - 1 ].IndexNum = ic - ModelObj->SubsetArray[ sc - 1 ].StartIndex;

			ModelObj->SubsetArray[ sc ].StartIndex = ic;


			bool materialFound = false;
			for( unsigned int i = 0; i < materialNum; i++ )
			{
				if (strncmp(str, materialArray[i].Name, sizeof(str)) == 0)
				{
					ModelObj->SubsetArray[ sc ].Material.Material = materialArray[i].Material;
					CopyToFixedBuffer(
						ModelObj->SubsetArray[sc].Material.TextureName,
						materialArray[i].TextureName,
						"OBJ texture path");
					CopyToFixedBuffer(
						ModelObj->SubsetArray[sc].Material.Name,
						materialArray[i].Name,
						"OBJ material name");

					materialFound = true;
					break;
				}
			}
			if (!materialFound)
			{
				throw std::runtime_error(std::string("OBJ material not found: ") + str);
			}

			sc++;
			
		}
		else if (TokenEquals(str, "f"))
		{
			//面
			in = 0;
			if (sc == 0)
			{
				ModelObj->SubsetArray[0].StartIndex = ic;
				sc = 1;
			}

			do
			{
				if (!ReadToken(file, str) || vc >= vertexNum || ic >= indexNum)
				{
					throw std::runtime_error("OBJ face exceeds allocated geometry");
				}

				const ObjVertexIndex vertexIndex = ParseObjVertexIndex(str);
				ModelObj->VertexArray[vc].Position = positionArray[
					ResolveObjIndex(vertexIndex.Position, positionNum, "position")];
				if (vertexIndex.TexCoord != 0)
				{
					ModelObj->VertexArray[vc].TexCoord = texcoordArray[
						ResolveObjIndex(vertexIndex.TexCoord, texcoordNum, "texture")];
				}
				if (vertexIndex.Normal != 0)
				{
					ModelObj->VertexArray[vc].Normal = normalArray[
						ResolveObjIndex(vertexIndex.Normal, normalNum, "normal")];
				}

				ModelObj->VertexArray[vc].Diffuse = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );

				ModelObj->IndexArray[ic] = vc;
				ic++;
				vc++;

				in++;
				c = fgetc( file );
			}
			while( c != '\n' && c != '\r' && c != EOF );

			//四角は三角に分割
			if( in == 4 )
			{
				if (ic + 1 >= indexNum)
				{
					throw std::runtime_error("OBJ quad index count is inconsistent");
				}
				ModelObj->IndexArray[ic] = vc - 4;
				ic++;
				ModelObj->IndexArray[ic] = vc - 2;
				ic++;
			}
		}
	}


	if( sc != 0 )
		ModelObj->SubsetArray[ sc - 1 ].IndexNum = ic - ModelObj->SubsetArray[ sc - 1 ].StartIndex;


	delete[] positionArray;
	delete[] normalArray;
	delete[] texcoordArray;
}




//マテリアル読み込み///////////////////////////////////////////////////////////////////
void ModelRenderer::LoadMaterial( const char *FileName, MODEL_MATERIAL **MaterialArray, unsigned int *MaterialNum )
{
	if (FileName == nullptr || MaterialArray == nullptr || MaterialNum == nullptr)
	{
		throw std::invalid_argument("ModelRenderer::LoadMaterial received a null argument");
	}

	const std::string directory = GetDirectory(FileName);



	char str[256];

	FILE *file{};
	if (fopen_s(&file, FileName, "rt") != 0 || file == nullptr)
	{
		throw std::runtime_error(std::string("MTL file not found: ") + FileName);
	}
	std::unique_ptr<FILE, decltype(&fclose)> fileHandle(file, &fclose);

	MODEL_MATERIAL *materialArray{};
	unsigned int materialNum = 0;

	//要素数カウント
	while (ReadToken(file, str))
	{
		if (TokenEquals(str, "newmtl"))
		{
			if (materialNum == UINT_MAX)
			{
				throw std::runtime_error("MTL material count exceeds supported limits");
			}
			materialNum++;
		}
	}

	if (materialNum == 0)
	{
		throw std::runtime_error("MTL file contains no materials");
	}

	//メモリ確保
	std::unique_ptr<MODEL_MATERIAL[]> materialOwner =
		std::make_unique<MODEL_MATERIAL[]>(materialNum);
	materialArray = materialOwner.get();


	//要素読込
	int mc = -1;

	fseek( file, 0, SEEK_SET );

	while (ReadToken(file, str))
	{
		if (TokenEquals(str, "newmtl"))
		{
			//マテリアル名
			mc++;
			if (mc < 0 || static_cast<unsigned int>(mc) >= materialNum ||
				!ReadToken(file, materialArray[mc].Name))
			{
				throw std::runtime_error("Invalid MTL material declaration");
			}

			materialArray[mc].Material.Emission.x = 0.0f;
			materialArray[mc].Material.Emission.y = 0.0f;
			materialArray[mc].Material.Emission.z = 0.0f;
			materialArray[mc].Material.Emission.w = 0.0f;
		}
		else if (TokenEquals(str, "Ka"))
		{
			if (mc < 0)
			{
				throw std::runtime_error("MTL property appears before newmtl");
			}
			//アンビエント
			if (fscanf(file, "%f%f%f",
				&materialArray[mc].Material.Ambient.x,
				&materialArray[mc].Material.Ambient.y,
				&materialArray[mc].Material.Ambient.z) != 3)
			{
				throw std::runtime_error("Invalid MTL ambient color");
			}
			materialArray[ mc ].Material.Ambient.w = 1.0f;
		}
		else if (TokenEquals(str, "Kd"))
		{
			//ディフューズ
			if (mc < 0 || fscanf(file, "%f%f%f",
				&materialArray[mc].Material.Diffuse.x,
				&materialArray[mc].Material.Diffuse.y,
				&materialArray[mc].Material.Diffuse.z) != 3)
			{
				throw std::runtime_error("Invalid MTL diffuse color");
			}
			materialArray[ mc ].Material.Diffuse.w = 1.0f;
		}
		else if (TokenEquals(str, "Ks"))
		{
			//スペキュラ
			if (mc < 0 || fscanf(file, "%f%f%f",
				&materialArray[mc].Material.Specular.x,
				&materialArray[mc].Material.Specular.y,
				&materialArray[mc].Material.Specular.z) != 3)
			{
				throw std::runtime_error("Invalid MTL specular color");
			}
			materialArray[ mc ].Material.Specular.w = 1.0f;
		}
		else if (TokenEquals(str, "Ns"))
		{
			//スペキュラ強度
			if (mc < 0 || fscanf(file, "%f", &materialArray[mc].Material.Shininess) != 1)
			{
				throw std::runtime_error("Invalid MTL shininess");
			}
		}
		else if (TokenEquals(str, "d"))
		{
			//アルファ
			if (mc < 0 || fscanf(file, "%f", &materialArray[mc].Material.Diffuse.w) != 1)
			{
				throw std::runtime_error("Invalid MTL alpha");
			}
		}
		else if (TokenEquals(str, "map_Kd"))
		{
			//テクスチャ
			if (mc < 0 || !ReadToken(file, str))
			{
				throw std::runtime_error("Invalid MTL texture path");
			}
			const std::string path = JoinPath(directory, str);
			CopyToFixedBuffer(materialArray[mc].TextureName, path, "MTL texture path");
		}
	}

	*MaterialArray = materialOwner.release();
	*MaterialNum = materialNum;
}

