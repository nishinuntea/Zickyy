#include "main.h"
#include "animationModel.h"
#include "renderer.h"
#include "gameObject.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace
{
	constexpr unsigned int MaxBoneInfluences = 4;
	constexpr double DefaultTicksPerSecond = 25.0;

	float Clamp01(float value)
	{
		return std::max(0.0f, std::min(value, 1.0f));
	}

	aiVector3D LerpVector(
		const aiVector3D& start,
		const aiVector3D& end,
		float amount)
	{
		return start + (end - start) * amount;
	}

	aiVector3D SampleVectorKeys(
		const aiVectorKey* keys,
		unsigned int keyCount,
		double time,
		double duration,
		const aiVector3D& fallback)
	{
		if (keys == nullptr || keyCount == 0)
		{
			return fallback;
		}
		if (keyCount == 1)
		{
			return keys[0].mValue;
		}

		for (unsigned int i = 0; i + 1 < keyCount; ++i)
		{
			if (time < keys[i + 1].mTime)
			{
				const double span = keys[i + 1].mTime - keys[i].mTime;
				const float amount = span > 0.0
					? Clamp01(static_cast<float>((time - keys[i].mTime) / span))
					: 0.0f;
				return LerpVector(keys[i].mValue, keys[i + 1].mValue, amount);
			}
		}

		const double wrapEnd = duration + keys[0].mTime;
		const double wrapSpan = wrapEnd - keys[keyCount - 1].mTime;
		if (wrapSpan > 0.0)
		{
			const float amount = Clamp01(static_cast<float>(
				(time - keys[keyCount - 1].mTime) / wrapSpan));
			return LerpVector(
				keys[keyCount - 1].mValue,
				keys[0].mValue,
				amount);
		}

		return keys[keyCount - 1].mValue;
	}

	aiQuaternion SampleQuaternionKeys(
		const aiQuatKey* keys,
		unsigned int keyCount,
		double time,
		double duration,
		const aiQuaternion& fallback)
	{
		if (keys == nullptr || keyCount == 0)
		{
			return fallback;
		}
		if (keyCount == 1)
		{
			return keys[0].mValue;
		}

		const aiQuaternion* start = &keys[keyCount - 1].mValue;
		const aiQuaternion* end = &keys[0].mValue;
		double startTime = keys[keyCount - 1].mTime;
		double endTime = duration + keys[0].mTime;
		for (unsigned int i = 0; i + 1 < keyCount; ++i)
		{
			if (time < keys[i + 1].mTime)
			{
				start = &keys[i].mValue;
				end = &keys[i + 1].mValue;
				startTime = keys[i].mTime;
				endTime = keys[i + 1].mTime;
				break;
			}
		}

		const double span = endTime - startTime;
		const float amount = span > 0.0
			? Clamp01(static_cast<float>((time - startTime) / span))
			: 0.0f;
		aiQuaternion result;
		aiQuaternion::Interpolate(result, *start, *end, amount);
		result.Normalize();
		return result;
	}

	aiMatrix4x4 BlendMatrices(
		const aiMatrix4x4& start,
		const aiMatrix4x4& end,
		float amount)
	{
		aiVector3D startScale;
		aiQuaternion startRotation;
		aiVector3D startPosition;
		start.Decompose(startScale, startRotation, startPosition);

		aiVector3D endScale;
		aiQuaternion endRotation;
		aiVector3D endPosition;
		end.Decompose(endScale, endRotation, endPosition);

		aiQuaternion rotation;
		aiQuaternion::Interpolate(
			rotation, startRotation, endRotation, Clamp01(amount));
		rotation.Normalize();
		return aiMatrix4x4(
			LerpVector(startScale, endScale, amount),
			rotation,
			LerpVector(startPosition, endPosition, amount));
	}

	std::string GetDirectory(const char* fileName)
	{
		const std::string path = fileName != nullptr ? fileName : "";
		const size_t separator = path.find_last_of("/\\");
		return separator == std::string::npos ? "" : path.substr(0, separator + 1);
	}

	bool IsAbsolutePath(const std::string& path)
	{
		return (path.size() >= 2 && path[1] == ':') ||
			(path.size() >= 2 && path[0] == '\\' && path[1] == '\\');
	}

	std::wstring ToWideString(const std::string& text)
	{
		if (text.empty())
		{
			return {};
		}

		UINT codePage = CP_UTF8;
		int length = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS,
			text.c_str(), -1, nullptr, 0);
		if (length == 0)
		{
			codePage = CP_ACP;
			length = MultiByteToWideChar(codePage, 0, text.c_str(), -1, nullptr, 0);
		}
		if (length <= 0)
		{
			return {};
		}

		std::wstring result(static_cast<size_t>(length), L'\0');
		MultiByteToWideChar(codePage, codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0,
			text.c_str(), -1, result.data(), length);
		result.pop_back();
		return result;
	}

	bool CreateEmbeddedTexture(
		const aiTexture* source,
		ID3D11ShaderResourceView** textureView)
	{
		if (source == nullptr || textureView == nullptr)
		{
			return false;
		}

		*textureView = nullptr;
		if (source->mHeight == 0)
		{
			TexMetadata metadata;
			ScratchImage image;
			const HRESULT loadResult = LoadFromWICMemory(
				reinterpret_cast<const uint8_t*>(source->pcData),
				source->mWidth,
				WIC_FLAGS_NONE,
				&metadata,
				image);
			if (FAILED(loadResult))
			{
				return false;
			}

			return SUCCEEDED(CreateShaderResourceView(
				Renderer::GetDevice(),
				image.GetImages(),
				image.GetImageCount(),
				metadata,
				textureView));
		}

		std::vector<unsigned char> pixels(
			static_cast<size_t>(source->mWidth) * source->mHeight * 4);
		for (unsigned int y = 0; y < source->mHeight; ++y)
		{
			for (unsigned int x = 0; x < source->mWidth; ++x)
			{
				const aiTexel& texel = source->pcData[y * source->mWidth + x];
				const size_t offset = (static_cast<size_t>(y) * source->mWidth + x) * 4;
				pixels[offset + 0] = texel.r;
				pixels[offset + 1] = texel.g;
				pixels[offset + 2] = texel.b;
				pixels[offset + 3] = texel.a;
			}
		}

		D3D11_TEXTURE2D_DESC description{};
		description.Width = source->mWidth;
		description.Height = source->mHeight;
		description.MipLevels = 1;
		description.ArraySize = 1;
		description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		description.SampleDesc.Count = 1;
		description.Usage = D3D11_USAGE_DEFAULT;
		description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initialData{};
		initialData.pSysMem = pixels.data();
		initialData.SysMemPitch = source->mWidth * 4;

		ID3D11Texture2D* texture{};
		HRESULT result = Renderer::GetDevice()->CreateTexture2D(
			&description, &initialData, &texture);
		if (SUCCEEDED(result))
		{
			result = Renderer::GetDevice()->CreateShaderResourceView(
				texture, nullptr, textureView);
		}
		if (texture != nullptr)
		{
			texture->Release();
		}
		return SUCCEEDED(result);
	}
}

struct AnimationModel::Impl
{
	struct Bone
	{
		aiMatrix4x4 BindMatrix;
		aiMatrix4x4 AnimationMatrix;
		aiMatrix4x4 OffsetMatrix;
		aiMatrix4x4 FinalMatrix;
		bool HasOffset{};
	};

	struct DeformVertex
	{
		aiVector3D Position;
		aiVector3D Normal;
		std::array<const Bone*, MaxBoneInfluences> BoneReferences{};
		std::array<float, MaxBoneInfluences> BoneWeights{};
	};

	struct Mesh
	{
		ID3D11Buffer* VertexBuffer{};
		ID3D11Buffer* IndexBuffer{};
		std::vector<VERTEX_3D> Vertices;
		std::vector<DeformVertex> DeformVertices;
		unsigned int IndexCount{};
		unsigned int MaterialIndex{};
	};

	struct Material
	{
		MATERIAL Value{};
		ID3D11ShaderResourceView* Texture{};
	};

	std::unique_ptr<Assimp::Importer> ModelImporter;
	const aiScene* Scene{};
	std::unordered_map<std::string, std::unique_ptr<Assimp::Importer>> AnimationImporters;
	std::unordered_map<std::string, const aiScene*> Animations;
	std::unordered_map<std::string, Bone> Bones;
	std::unordered_map<std::string, aiMatrix4x4> TargetMatrices;
	std::unordered_map<std::string, aiMatrix4x4> TransitionStartMatrices;
	std::vector<Mesh> Meshes;
	std::vector<Material> Materials;
	aiMatrix4x4 GlobalInverse;
	Vector3 LocalRotation{};
	float LocalScale{ 1.0f };
	std::string CurrentAnimation;
	double AnimationTimeTicks{};
	float TransitionElapsed{};
	float TransitionDuration{ 0.18f };
	bool Loaded{};
	bool Transitioning{};

	~Impl()
	{
		Clear();
	}

	void Clear()
	{
		for (Mesh& mesh : Meshes)
		{
			if (mesh.VertexBuffer != nullptr) mesh.VertexBuffer->Release();
			if (mesh.IndexBuffer != nullptr) mesh.IndexBuffer->Release();
		}
		for (Material& material : Materials)
		{
			if (material.Texture != nullptr) material.Texture->Release();
		}

		Meshes.clear();
		Materials.clear();
		Bones.clear();
		TargetMatrices.clear();
		TransitionStartMatrices.clear();
		Animations.clear();
		AnimationImporters.clear();
		CurrentAnimation.clear();
		AnimationTimeTicks = 0.0;
		TransitionElapsed = 0.0f;
		Transitioning = false;
		Scene = nullptr;
		ModelImporter.reset();
		Loaded = false;
	}

	void RegisterNodes(const aiNode* node)
	{
		if (node == nullptr)
		{
			return;
		}

		Bone& bone = Bones[node->mName.C_Str()];
		bone.BindMatrix = node->mTransformation;
		bone.AnimationMatrix = bone.BindMatrix;
		TargetMatrices[node->mName.C_Str()] = bone.BindMatrix;
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			RegisterNodes(node->mChildren[i]);
		}
	}

	void AddBoneInfluence(
		DeformVertex& vertex,
		const Bone* bone,
		float weight)
	{
		if (bone == nullptr || weight <= 0.0f)
		{
			return;
		}

		unsigned int target = MaxBoneInfluences;
		for (unsigned int i = 0; i < MaxBoneInfluences; ++i)
		{
			if (vertex.BoneWeights[i] == 0.0f)
			{
				target = i;
				break;
			}
		}

		if (target == MaxBoneInfluences)
		{
			target = 0;
			for (unsigned int i = 1; i < MaxBoneInfluences; ++i)
			{
				if (vertex.BoneWeights[i] < vertex.BoneWeights[target])
				{
					target = i;
				}
			}
			if (weight <= vertex.BoneWeights[target])
			{
				return;
			}
		}

		vertex.BoneReferences[target] = bone;
		vertex.BoneWeights[target] = weight;
	}

	void NormalizeBoneWeights(DeformVertex& vertex)
	{
		float total{};
		for (float weight : vertex.BoneWeights)
		{
			total += weight;
		}
		if (total <= 0.0f)
		{
			return;
		}
		for (float& weight : vertex.BoneWeights)
		{
			weight /= total;
		}
	}

	bool LoadTexture(
		const aiScene* scene,
		const aiString& textureName,
		const std::string& directory,
		ID3D11ShaderResourceView** texture)
	{
		if (texture == nullptr)
		{
			return false;
		}
		*texture = nullptr;

		if (const aiTexture* embedded = scene->GetEmbeddedTexture(textureName.C_Str()))
		{
			return CreateEmbeddedTexture(embedded, texture);
		}

		std::string path = textureName.C_Str();
		std::replace(path.begin(), path.end(), '/', '\\');
		if (!IsAbsolutePath(path))
		{
			path = directory + path;
		}
		const std::wstring widePath = ToWideString(path);
		if (widePath.empty())
		{
			return false;
		}

		try
		{
			Renderer::CreateTextureFromFile(texture, widePath.c_str());
			return *texture != nullptr;
		}
		catch (...)
		{
			SafeRelease(*texture);
			return false;
		}
	}

	bool CreateMaterials(const char* fileName)
	{
		Materials.resize(Scene->mNumMaterials);
		const std::string directory = GetDirectory(fileName);
		for (unsigned int i = 0; i < Scene->mNumMaterials; ++i)
		{
			Material& destination = Materials[i];
			destination.Value.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
			destination.Value.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			destination.Value.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

			aiColor4D color;
			if (AI_SUCCESS == aiGetMaterialColor(
				Scene->mMaterials[i], AI_MATKEY_COLOR_DIFFUSE, &color))
			{
				destination.Value.Diffuse = XMFLOAT4(color.r, color.g, color.b, color.a);
			}

			aiString textureName;
			if (AI_SUCCESS == Scene->mMaterials[i]->GetTexture(
				aiTextureType_DIFFUSE, 0, &textureName))
			{
				LoadTexture(Scene, textureName, directory, &destination.Texture);
			}
			destination.Value.TextureEnable = destination.Texture != nullptr;
		}
		return true;
	}

	bool CreateMeshes()
	{
		Meshes.reserve(Scene->mNumMeshes);
		for (unsigned int meshIndex = 0; meshIndex < Scene->mNumMeshes; ++meshIndex)
		{
			const aiMesh* source = Scene->mMeshes[meshIndex];
			if (source == nullptr || source->mNumVertices == 0)
			{
				continue;
			}

			Mesh destination;
			destination.MaterialIndex = source->mMaterialIndex;
			destination.Vertices.resize(source->mNumVertices);
			destination.DeformVertices.resize(source->mNumVertices);
			for (unsigned int i = 0; i < source->mNumVertices; ++i)
			{
				const aiVector3D position = source->mVertices[i];
				const aiVector3D normal = source->HasNormals()
					? source->mNormals[i]
					: aiVector3D(0.0f, 1.0f, 0.0f);
				const aiVector3D textureCoordinate = source->HasTextureCoords(0)
					? source->mTextureCoords[0][i]
					: aiVector3D();

				destination.Vertices[i].Position = XMFLOAT3(position.x, position.y, position.z);
				destination.Vertices[i].Normal = XMFLOAT3(normal.x, normal.y, normal.z);
				destination.Vertices[i].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				destination.Vertices[i].TexCoord = XMFLOAT2(
					textureCoordinate.x, textureCoordinate.y);
				destination.DeformVertices[i].Position = position;
				destination.DeformVertices[i].Normal = normal;
			}

			for (unsigned int boneIndex = 0; boneIndex < source->mNumBones; ++boneIndex)
			{
				const aiBone* sourceBone = source->mBones[boneIndex];
				const std::string boneName = sourceBone->mName.C_Str();
				Bone& bone = Bones[boneName];
				bone.OffsetMatrix = sourceBone->mOffsetMatrix;
				bone.HasOffset = true;

				for (unsigned int weightIndex = 0;
					weightIndex < sourceBone->mNumWeights; ++weightIndex)
				{
					const aiVertexWeight& sourceWeight = sourceBone->mWeights[weightIndex];
					if (sourceWeight.mVertexId < destination.DeformVertices.size())
					{
						AddBoneInfluence(
							destination.DeformVertices[sourceWeight.mVertexId],
							&bone,
							sourceWeight.mWeight);
					}
				}
			}

			for (DeformVertex& vertex : destination.DeformVertices)
			{
				NormalizeBoneWeights(vertex);
			}

			std::vector<unsigned int> indices;
			indices.reserve(static_cast<size_t>(source->mNumFaces) * 3);
			for (unsigned int faceIndex = 0; faceIndex < source->mNumFaces; ++faceIndex)
			{
				const aiFace& face = source->mFaces[faceIndex];
				for (unsigned int i = 0; i < face.mNumIndices; ++i)
				{
					indices.push_back(face.mIndices[i]);
				}
			}
			destination.IndexCount = static_cast<unsigned int>(indices.size());

			D3D11_BUFFER_DESC vertexDescription{};
			vertexDescription.Usage = D3D11_USAGE_DYNAMIC;
			vertexDescription.ByteWidth = static_cast<UINT>(
				destination.Vertices.size() * sizeof(VERTEX_3D));
			vertexDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA vertexData{};
			vertexData.pSysMem = destination.Vertices.data();
			if (FAILED(Renderer::GetDevice()->CreateBuffer(
				&vertexDescription, &vertexData, &destination.VertexBuffer)))
			{
				return false;
			}

			D3D11_BUFFER_DESC indexDescription{};
			indexDescription.Usage = D3D11_USAGE_DEFAULT;
			indexDescription.ByteWidth = static_cast<UINT>(
				indices.size() * sizeof(unsigned int));
			indexDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA indexData{};
			indexData.pSysMem = indices.data();
			if (indices.empty() || FAILED(Renderer::GetDevice()->CreateBuffer(
				&indexDescription, &indexData, &destination.IndexBuffer)))
			{
				if (destination.VertexBuffer != nullptr)
				{
					destination.VertexBuffer->Release();
				}
				return false;
			}

			Meshes.push_back(std::move(destination));
		}
		return !Meshes.empty();
	}

	void UpdateBoneMatrices(const aiNode* node, const aiMatrix4x4& parentMatrix)
	{
		if (node == nullptr)
		{
			return;
		}

		const auto boneIterator = Bones.find(node->mName.C_Str());
		const aiMatrix4x4 localMatrix = boneIterator != Bones.end()
			? boneIterator->second.AnimationMatrix
			: node->mTransformation;
		const aiMatrix4x4 worldMatrix = parentMatrix * localMatrix;
		if (boneIterator != Bones.end() && boneIterator->second.HasOffset)
		{
			boneIterator->second.FinalMatrix =
				GlobalInverse * worldMatrix * boneIterator->second.OffsetMatrix;
		}

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			UpdateBoneMatrices(node->mChildren[i], worldMatrix);
		}
	}

	void SkinVertices()
	{
		for (Mesh& mesh : Meshes)
		{
			for (size_t vertexIndex = 0;
				vertexIndex < mesh.DeformVertices.size(); ++vertexIndex)
			{
				const DeformVertex& source = mesh.DeformVertices[vertexIndex];
				aiMatrix4x4 skinMatrix;
				for (unsigned int row = 0; row < 4; ++row)
				{
					for (unsigned int column = 0; column < 4; ++column)
					{
						skinMatrix[row][column] = 0.0f;
					}
				}

				float totalWeight{};
				for (unsigned int influence = 0;
					influence < MaxBoneInfluences; ++influence)
				{
					const float weight = source.BoneWeights[influence];
					if (weight <= 0.0f)
					{
						continue;
					}

					const Bone* bone = source.BoneReferences[influence];
					if (bone == nullptr)
					{
						continue;
					}
					const aiMatrix4x4 weightedMatrix =
						bone->FinalMatrix * weight;
					for (unsigned int row = 0; row < 4; ++row)
					{
						for (unsigned int column = 0; column < 4; ++column)
						{
							skinMatrix[row][column] += weightedMatrix[row][column];
						}
					}
					totalWeight += weight;
				}

				aiVector3D position = source.Position;
				aiVector3D normal = source.Normal;
				if (totalWeight > 0.0f)
				{
					position = skinMatrix * position;
					skinMatrix.a4 = 0.0f;
					skinMatrix.b4 = 0.0f;
					skinMatrix.c4 = 0.0f;
					normal = skinMatrix * normal;
					normal.Normalize();
				}

				mesh.Vertices[vertexIndex].Position =
					XMFLOAT3(position.x, position.y, position.z);
				mesh.Vertices[vertexIndex].Normal =
					XMFLOAT3(normal.x, normal.y, normal.z);
			}

			D3D11_MAPPED_SUBRESOURCE mapped{};
			if (SUCCEEDED(Renderer::GetDeviceContext()->Map(
				mesh.VertexBuffer,
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mapped)))
			{
				memcpy(
					mapped.pData,
					mesh.Vertices.data(),
					mesh.Vertices.size() * sizeof(VERTEX_3D));
				Renderer::GetDeviceContext()->Unmap(mesh.VertexBuffer, 0);
			}
		}
	}
};

AnimationModel::AnimationModel(GameObject* gameObject)
	: Component(gameObject), m_Impl(std::make_unique<Impl>())
{
}

AnimationModel::~AnimationModel() = default;

void AnimationModel::Init()
{
}

void AnimationModel::Uninit()
{
	m_Impl->Clear();
}

bool AnimationModel::Load(const char* fileName)
{
	if (fileName == nullptr || Renderer::GetDevice() == nullptr)
	{
		return false;
	}

	m_Impl->Clear();
	m_Impl->ModelImporter = std::make_unique<Assimp::Importer>();
	const unsigned int flags =
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		aiProcess_LimitBoneWeights |
		aiProcess_ConvertToLeftHanded;
	m_Impl->Scene = m_Impl->ModelImporter->ReadFile(fileName, flags);
	if (m_Impl->Scene == nullptr || m_Impl->Scene->mRootNode == nullptr)
	{
		m_Impl->Clear();
		return false;
	}

	m_Impl->GlobalInverse = m_Impl->Scene->mRootNode->mTransformation;
	m_Impl->GlobalInverse.Inverse();
	m_Impl->RegisterNodes(m_Impl->Scene->mRootNode);
	if (!m_Impl->CreateMaterials(fileName) || !m_Impl->CreateMeshes())
	{
		m_Impl->Clear();
		return false;
	}

	m_Impl->UpdateBoneMatrices(m_Impl->Scene->mRootNode, aiMatrix4x4());
	m_Impl->SkinVertices();
	m_Impl->Loaded = true;
	return true;
}

bool AnimationModel::LoadAnimation(
	const char* fileName,
	const char* animationName)
{
	if (!m_Impl->Loaded || fileName == nullptr || animationName == nullptr ||
		animationName[0] == '\0')
	{
		return false;
	}

	auto importer = std::make_unique<Assimp::Importer>();
	const aiScene* animationScene = importer->ReadFile(
		fileName,
		aiProcess_ConvertToLeftHanded);
	if (animationScene == nullptr || !animationScene->HasAnimations())
	{
		return false;
	}

	const std::string name(animationName);
	m_Impl->Animations[name] = animationScene;
	m_Impl->AnimationImporters[name] = std::move(importer);
	return true;
}

void AnimationModel::Update(const char* animationName, float deltaTime)
{
	if (!m_Impl->Loaded || animationName == nullptr)
	{
		return;
	}

	const auto animationScene = m_Impl->Animations.find(animationName);
	if (animationScene == m_Impl->Animations.end() ||
		!animationScene->second->HasAnimations())
	{
		return;
	}

	if (deltaTime < 0.0f)
	{
		deltaTime = 0.0f;
	}

	const aiAnimation* animation = animationScene->second->mAnimations[0];
	const std::string requestedAnimation(animationName);
	if (requestedAnimation != m_Impl->CurrentAnimation)
	{
		m_Impl->TransitionStartMatrices.clear();
		for (const auto& pair : m_Impl->Bones)
		{
			m_Impl->TransitionStartMatrices[pair.first] =
				pair.second.AnimationMatrix;
		}
		m_Impl->CurrentAnimation = requestedAnimation;
		m_Impl->AnimationTimeTicks = 0.0;
		m_Impl->TransitionElapsed = 0.0f;
		m_Impl->Transitioning = true;
	}

	const double ticksPerSecond = animation->mTicksPerSecond > 0.0
		? animation->mTicksPerSecond
		: DefaultTicksPerSecond;
	if (animation->mDuration > 0.0)
	{
		m_Impl->AnimationTimeTicks = std::fmod(
			m_Impl->AnimationTimeTicks + deltaTime * ticksPerSecond,
			animation->mDuration);
	}

	for (const auto& pair : m_Impl->Bones)
	{
		m_Impl->TargetMatrices[pair.first] = pair.second.BindMatrix;
	}

	for (unsigned int channelIndex = 0;
		channelIndex < animation->mNumChannels; ++channelIndex)
	{
		const aiNodeAnim* channel = animation->mChannels[channelIndex];
		const auto bone = m_Impl->Bones.find(channel->mNodeName.C_Str());
		if (bone == m_Impl->Bones.end())
		{
			continue;
		}

		aiVector3D bindScale;
		aiQuaternion bindRotation;
		aiVector3D bindPosition;
		bone->second.BindMatrix.Decompose(
			bindScale, bindRotation, bindPosition);

		const aiVector3D scale = SampleVectorKeys(
			channel->mScalingKeys,
			channel->mNumScalingKeys,
			m_Impl->AnimationTimeTicks,
			animation->mDuration,
			bindScale);
		const aiQuaternion rotation = SampleQuaternionKeys(
			channel->mRotationKeys,
			channel->mNumRotationKeys,
			m_Impl->AnimationTimeTicks,
			animation->mDuration,
			bindRotation);
		const aiVector3D position = SampleVectorKeys(
			channel->mPositionKeys,
			channel->mNumPositionKeys,
			m_Impl->AnimationTimeTicks,
			animation->mDuration,
			bindPosition);

		m_Impl->TargetMatrices[channel->mNodeName.C_Str()] =
			aiMatrix4x4(scale, rotation, position);
	}

	float blendAmount = 1.0f;
	if (m_Impl->Transitioning)
	{
		m_Impl->TransitionElapsed += deltaTime;
		blendAmount = m_Impl->TransitionDuration > 0.0f
			? Clamp01(m_Impl->TransitionElapsed / m_Impl->TransitionDuration)
			: 1.0f;
		blendAmount = blendAmount * blendAmount * (3.0f - 2.0f * blendAmount);
	}

	for (auto& pair : m_Impl->Bones)
	{
		const aiMatrix4x4& target = m_Impl->TargetMatrices[pair.first];
		if (m_Impl->Transitioning)
		{
			const auto start = m_Impl->TransitionStartMatrices.find(pair.first);
			pair.second.AnimationMatrix = start != m_Impl->TransitionStartMatrices.end()
				? BlendMatrices(start->second, target, blendAmount)
				: target;
		}
		else
		{
			pair.second.AnimationMatrix = target;
		}
	}

	if (m_Impl->Transitioning && blendAmount >= 1.0f)
	{
		m_Impl->Transitioning = false;
		m_Impl->TransitionStartMatrices.clear();
	}

	m_Impl->UpdateBoneMatrices(m_Impl->Scene->mRootNode, aiMatrix4x4());
	m_Impl->SkinVertices();
}

void AnimationModel::Draw()
{
	if (!m_Impl->Loaded || m_GameObject == nullptr)
	{
		return;
	}

	const XMMATRIX localScale = XMMatrixScaling(
		m_Impl->LocalScale, m_Impl->LocalScale, m_Impl->LocalScale);
	const XMMATRIX localRotation = XMMatrixRotationRollPitchYaw(
		m_Impl->LocalRotation.x,
		m_Impl->LocalRotation.y,
		m_Impl->LocalRotation.z);
	Renderer::SetWorldMatrix(
		localScale * localRotation * m_GameObject->GetMatrix());

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (const Impl::Mesh& mesh : m_Impl->Meshes)
	{
		Renderer::GetDeviceContext()->IASetVertexBuffers(
			0, 1, &mesh.VertexBuffer, &stride, &offset);
		Renderer::GetDeviceContext()->IASetIndexBuffer(
			mesh.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		ID3D11ShaderResourceView* texture{};
		MATERIAL material{};
		material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		if (mesh.MaterialIndex < m_Impl->Materials.size())
		{
			const Impl::Material& source = m_Impl->Materials[mesh.MaterialIndex];
			material = source.Value;
			texture = source.Texture;
		}
		Renderer::SetMaterial(material);
		Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &texture);
		Renderer::GetDeviceContext()->DrawIndexed(mesh.IndexCount, 0, 0);
	}
}

bool AnimationModel::IsLoaded() const
{
	return m_Impl->Loaded;
}

void AnimationModel::SetLocalScale(float scale)
{
	if (scale > 0.0f)
	{
		m_Impl->LocalScale = scale;
	}
}

void AnimationModel::SetLocalRotation(const Vector3& rotation)
{
	m_Impl->LocalRotation = rotation;
}
