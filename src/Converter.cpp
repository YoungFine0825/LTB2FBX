
#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"


#include "Converter.h"

#include <vector>

#define DECODING_TEMP_FILE_PATH "__ltb_decoded.temp"

using namespace std;

typedef vector<aiVector3D> Vector3List;
typedef vector<aiFace> FaceList;
typedef vector<aiVertexWeight> WeightList;
typedef vector<aiNode> NodeList;
typedef vector<aiNode*> NodePtrList;

Converter::Converter() 
{
	m_lzmaDecoder = new LzmaDecoder();
}

Converter::~Converter() 
{
	m_lzmaDecoder->Destroy();
	m_meshesList.clear();
	m_ltbDrawableList.clear();
	m_skeletonNodes.clear();
	m_name2SkeNode.clear();
	m_name2SkeNodeIdx.clear();
	m_bonesList.clear();
	m_meshBonesArray.clear();
	m_numBonesPerMesh.clear();
}

int Converter::ConvertSingleLTBFile(const std::string& ltbFilePath, const std::string& outFilePath)
{
	Model* ltbModel = new Model();
	int ret = LoadLTBModel(ltbFilePath,ltbModel);
	if (ret != CONVERT_RET_OK) 
	{
		delete ltbModel;
		return ret;
	}
	bool success = doConvertLTB(ltbModel, outFilePath);
	if (!success)
	{
		delete ltbModel;
		return CONVERT_RET_EXPORT_LTB_2_FBX_FAILED;
	}
	delete ltbModel;
	return CONVERT_RET_OK;
}

bool Converter::doConvertLTB(Model* ltbModel, std::string outFilePath)
{
	if (exportScene) 
	{
		delete exportScene;
	}
	exportScene = new aiScene();
	//
	const unsigned int numMeshes = ltbModel->m_Pieces.GetSize();
	//
	grabSkeletonNodesFromLTB(ltbModel);
	//
	grabMeshesFromLTB(ltbModel);
	//
	grabAndBuildMeshBones(ltbModel);
	//
	exportScene->mRootNode = new aiNode();
	exportScene->mRootNode->mName = ltbModel->GetFilename();
	exportScene->mNumMeshes = numMeshes;
	exportScene->mMeshes = &m_meshesList[0];
	//
	std::vector<aiNode*> meshNodes;
	std::vector<aiMaterial*> materials;
	for (size_t i = 0; i < numMeshes; ++i)
	{
		aiNode* node = new aiNode();
		node->mName = m_meshesList[i]->mName;
		node->mNumMeshes = 1;
		node->mMeshes = new unsigned int[1];
		node->mMeshes[0] = i;
		meshNodes.push_back(node);
		exportScene->mRootNode->addChildren(1, &node);
		printf("Export Mesh : %s\n", node->mName.data);
		//
		aiBone* arrayHead = m_meshBonesArray[i];
		m_meshesList[i]->mBones = &arrayHead;
		m_meshesList[i]->mNumBones = m_numBonesPerMesh[i];
		//
		aiMaterial* mat = new aiMaterial();
		materials.push_back(mat);
	}
	//
	exportScene->mMaterials = &materials[0];
	exportScene->mNumMaterials = numMeshes;
	//
	aiNode* nodesArray = &m_skeletonNodes[0];
	exportScene->mRootNode->addChildren(1, &nodesArray);
	//
	exportScene->mNumAnimations = 0;
	exportScene->mNumCameras = 0;
	exportScene->mNumLights = 0;
	exportScene->mNumTextures = 0;
	//
	Assimp::Exporter exporter;
	aiReturn ret = exporter.Export(exportScene, "fbx", outFilePath);
	if (ret != aiReturn_SUCCESS) 
	{
		return false;
	}
	return true;
}

void Converter::grabSkeletonNodesFromLTB(Model* ltbModel)
{
	m_skeletonNodes.clear();
	m_name2SkeNode.clear();
	m_name2SkeNodeIdx.clear();
	m_bonesList.clear();
	//
	unsigned int numNodes = ltbModel->m_FlatNodeList.GetSize();
	//
	for (unsigned int nIdx = 0; nIdx < numNodes; ++nIdx)
	{
		aiNode* skeNode = new aiNode();
		ModelNode* ltbNode = ltbModel->GetNode(nIdx);
		//
		string nodeName = ltbNode->GetName();
		skeNode->mName = nodeName;
		m_name2SkeNode[nodeName] = skeNode;
		m_name2SkeNodeIdx[nodeName] = nIdx;
		//
		LTMatrix ltbMat = ltbNode->GetFromParentTransform();
		aiMatrix4x4 mat(
			ltbMat.m[0][0], ltbMat.m[0][1], ltbMat.m[0][2], ltbMat.m[0][3],
			ltbMat.m[1][0], ltbMat.m[1][1], ltbMat.m[1][2], ltbMat.m[1][3],
			ltbMat.m[2][0], ltbMat.m[2][1], ltbMat.m[2][2], ltbMat.m[2][3],
			ltbMat.m[3][0], ltbMat.m[3][1], ltbMat.m[3][2], ltbMat.m[3][3]
		);
		skeNode->mTransformation = mat;
		//
		m_skeletonNodes.push_back(*skeNode);
		//
		aiBone bone;
		bone.mName = nodeName;
		bone.mNumWeights = 0;
		bone.mWeights = nullptr;
		bone.mNode = skeNode;
		bone.mArmature = skeNode;
		bone.mOffsetMatrix = mat;
		m_bonesList.push_back(bone);
	}
	//
	vector<vector<aiNode*>> childrenList(numNodes);
	for (size_t nIdx = 0; nIdx < numNodes; ++nIdx) 
	{
		ModelNode* ltbNode = ltbModel->GetNode(nIdx);
		if (ltbNode->m_iParentNode != NODEPARENT_NONE) 
		{
			ModelNode* parentLtbMode = ltbModel->GetNode(ltbNode->m_iParentNode);
			aiNode* parentSkeNode = m_name2SkeNode[parentLtbMode->GetName()];
			aiNode* skeNode = &m_skeletonNodes[nIdx];
			skeNode->mParent = parentSkeNode;
			childrenList[ltbNode->m_iParentNode].push_back(skeNode);
		}
	}
	//
	for (size_t nIdx = 0; nIdx < numNodes; ++nIdx)
	{
		aiNode* node = &m_skeletonNodes[nIdx];
		size_t childCount = childrenList[nIdx].size();
		if (childCount > 0) 
		{
			node->addChildren(childCount, &childrenList[nIdx][0]);
		}
	}
}

void Converter::grabMeshesFromLTB(Model* ltbModel)
{
	m_meshesList.clear();
	m_ltbDrawableList.clear();
	//
	const unsigned int numMeshes = ltbModel->m_Pieces.GetSize();
	//
	for (unsigned int meshIdx = 0; meshIdx < numMeshes; ++meshIdx)
	{
		ModelPiece* piece = ltbModel->GetPiece(meshIdx);
		CDIModelDrawable* draw = piece->GetLOD(0);
		//
		aiMesh* mesh = new aiMesh();
		mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
		mesh->mName.Set(piece->GetName());
		//
		size_t numVert = draw->m_Verts.GetSize();
		aiVector3D* vertices = new aiVector3D[numVert]();
		aiVector3D* normals = new aiVector3D[numVert]();
		aiVector3D* uvs = new aiVector3D[numVert]();
		aiVector3D* tangents = new aiVector3D[numVert]();
		aiVector3D* bitangents = new aiVector3D[numVert]();
		for (size_t vIdx = 0; vIdx < numVert; ++vIdx)
		{
			ModelVert ltbVert = draw->m_Verts[vIdx];
			aiVector3D vert, uv, normal, tangent, bitangent;
			vert.x = ltbVert.m_Vec.x; vert.y = ltbVert.m_Vec.y; vert.z = ltbVert.m_Vec.z;
			uv.x = ltbVert.m_Uv.tu; uv.y = ltbVert.m_Uv.tv;
			normal.x = ltbVert.m_Normal.x; normal.y = ltbVert.m_Normal.y; normal.z = ltbVert.m_Normal.z;
			tangent.x = 0; tangent.y = 0; tangent.z = 0;
			bitangent.x = 0; bitangent.y = 0; bitangent.z = 0;
			vertices[vIdx] = vert;
			normals[vIdx] = normal;
			uvs[vIdx] = uv;
			tangents[vIdx] = tangent;
			bitangents[vIdx] = bitangent;
		}
		size_t numTri = draw->m_Tris.GetSize();
		aiFace* faces = new aiFace[numTri]();
		for (size_t faceIdx = 0; faceIdx < numTri; ++faceIdx)
		{
			ModelTri tri = draw->m_Tris[faceIdx];
			faces[faceIdx].mNumIndices = 3;
			faces[faceIdx].mIndices = new unsigned int[3]{ tri.m_Indices[0] ,tri.m_Indices[1] ,tri.m_Indices[2] };
		}
		//
		mesh->mNumVertices = numVert;
		mesh->mVertices = vertices;
		mesh->mNormals = normals;
		mesh->mTextureCoords[0] = uvs;
		mesh->mNumUVComponents[0] = 2;
		mesh->mTangents = tangents;
		mesh->mBitangents = bitangents;
		mesh->mFaces = faces;
		mesh->mNumFaces = numTri;
		mesh->mNumBones = 0;
		mesh->mNumAnimMeshes = 0;
		//
		m_meshesList.push_back(mesh);
		m_ltbDrawableList.push_back(draw);
	}
}

void Converter::grabAndBuildMeshBones(Model* ltbModel)
{
	m_meshBonesArray.clear();
	m_numBonesPerMesh.clear();
	unsigned int numMeshes = m_meshesList.size();
	//
	for (unsigned int meshIdx = 0; meshIdx < numMeshes; ++meshIdx)
	{
		aiMesh* mesh = m_meshesList[meshIdx];
		CDIModelDrawable* drawable = m_ltbDrawableList[meshIdx];
		unsigned int numVertices = mesh->mNumVertices;
		//
		vector<string> bonesName;
		map<string, WeightList> weights;
		//提取LTB模型顶点上的权重数据
		for (unsigned int vIdx = 0; vIdx < numVertices; ++vIdx)
		{
			ModelVert ltbModelVert = drawable->m_Verts[vIdx];
			unsigned int numBones = ltbModelVert.m_NumBones;
			Weights ltbWeight = ltbModelVert.m_Weights;
			for (unsigned int b = 0; b < numBones; ++b)
			{
				unsigned int boneIdx = ltbWeight.m_iBone[b];
				float weight = ltbWeight.m_fWeight[b];
				if (boneIdx >= 0) 
				{
					string ltbNodeName = ltbModel->GetNode(boneIdx)->GetName();
					aiVertexWeight wei;
					wei.mVertexId = vIdx;
					wei.mWeight = weight;
					//
					map<string, WeightList>::iterator i = weights.find(ltbNodeName);
					if (i == weights.end())
					{
						bonesName.push_back(ltbNodeName);
						//
						WeightList newWeiList(1,wei);
						weights[ltbNodeName] = newWeiList;
					}
					else 
					{
						i->second.push_back(wei);
					}
				}
			}
		}
		//
		//构建当前网格所需的骨骼数据
		unsigned int numBonesOfMesh = bonesName.size();
		aiBone* boneArray = new aiBone[numBonesOfMesh]();
		for (unsigned int boneIdx = 0; boneIdx < numBonesOfMesh; ++boneIdx)
		{
			string boneName = bonesName[boneIdx];
			aiBone* bone = &boneArray[boneIdx];
			bone->mName.Set(boneName.c_str());
			map<string, WeightList>::iterator i = weights.find(boneName);
			if (i != weights.end()) 
			{
				bone->mNumWeights = i->second.size();
				bone->mWeights = &(i->second[0]);
			}
			aiNode* sceneNode = getSkeletonNodeByName(boneName);
			if (sceneNode) 
			{
				bone->mNode = sceneNode;
				bone->mArmature = sceneNode;
				bone->mOffsetMatrix = sceneNode->mTransformation;
			}
		}
		//
		if (numBonesOfMesh > 0) 
		{
			//保存网格所需的骨骼数据
			m_meshBonesArray.push_back(boneArray);
			m_numBonesPerMesh.push_back(numBonesOfMesh);
		}
	}
}

aiNode* Converter::getSkeletonNodeByName(const string& name) 
{
	std::map<std::string, aiNode*>::iterator i = m_name2SkeNode.find(name);
	if (i == m_name2SkeNode.end()) 
	{
		return nullptr;
	}
	return i->second;
}

aiBone* Converter::getBoneByName(const string& name) 
{
	std::map<std::string, int>::iterator i = m_name2SkeNodeIdx.find(name);
	if (i == m_name2SkeNodeIdx.end())
	{
		return nullptr;
	}
	int idx = i->second;
	aiBone* ret = &m_bonesList[idx];
	return ret;
}

int Converter::LoadLTBModel(const std::string& modelFilePath, Model* ltbModel)
{
	DosFileStream* inStream = new DosFileStream();
	if (inStream->Open(modelFilePath.c_str()) != LT_OK)
	{
		delete inStream;
		return CONVERT_RET_INVALID_INPUT_FILE;
	}

	LTB_Header header;
	if (!readLTBHeader(&header, inStream))
	{
		inStream->Close();
		delete inStream;
		return CONVERT_RET_INVALID_HEADER;
	}

	bool isLzmaFile = header.m_iFileType > 20;
	if (isLzmaFile)
	{
		inStream->Close();
		delete inStream;
		//
		inStream = new DosFileStream();
		if (!decodingLTBFile(modelFilePath, inStream))
		{
			delete inStream;
			return CONVERT_RET_DECODING_FAILED;
		}
	}

	ModelLoadRequest* pRequest = new ModelLoadRequest();
	if (!pRequest)
	{
		inStream->Close();
		delete inStream;
		return CONVERT_RET_LOADING_MODEL_FAILED;
	}

	pRequest->m_pFile = inStream;
	pRequest->m_pFilename = modelFilePath.c_str();

	LTRESULT loadRet = ltbModel->Load(pRequest, modelFilePath.c_str());
	if (loadRet != LT_OK)
	{
		inStream->Close();
		delete inStream;
		delete pRequest;
		return CONVERT_RET_LOADING_MODEL_FAILED;
	}
	inStream->Close();
	delete inStream;
	delete pRequest;
	return CONVERT_RET_OK;
}

bool Converter::readLTBHeader(LTB_Header* head, DosFileStream* stream)
{
	if (stream->Read(head, sizeof(LTB_Header)) != LT_OK) 
	{
		return false;
	}
	fseek(stream->m_pFile, 0, SEEK_SET);//重新定位到文件头
	return true;
}

#define OUT_FILE_SIZE (1 << 24)

bool Converter::decodingLTBFile(const std::string& ltbFilePath, DosFileStream* fileStream)
{
	FILE* f = fopen(DECODING_TEMP_FILE_PATH, "w");
	if (f)
	{
		fclose(f);
	}
	int ret = m_lzmaDecoder->Decode(ltbFilePath.c_str(), DECODING_TEMP_FILE_PATH);
	if (ret != DEC_RET_SUCCESSFUL)
	{
		return false;
	}
	if (fileStream->Open(DECODING_TEMP_FILE_PATH) != LT_OK)
	{
		return false;
	}
	return true;
}