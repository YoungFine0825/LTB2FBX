
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
	releaseGrabbedData();
}

void Converter::releaseGrabbedData() 
{
	m_ltbDrawableList.clear();
	//
	if (m_meshesPtrVec.size() > 0) 
	{
		for (size_t i = 0; i < m_meshesPtrVec.size(); ++i) 
		{
			delete m_meshesPtrVec[i];
		}
	}
	m_meshesPtrVec.clear();
	//
	m_name2SkeNode.clear();
	m_name2SkeNodeIdx.clear();
	m_skeletonNodes.clear();
	//
	if (m_bonesPtrArrPerMeshVec.size() > 0)
	{
		for (size_t i = 0; i < m_bonesPtrArrPerMeshVec.size(); ++i)
		{
			for (size_t j = 0; j < m_bonesPtrArrPerMeshVec[i].size(); ++j)
			{
				delete m_bonesPtrArrPerMeshVec[i][j];
			}
		}
	}
	m_bonesPtrArrPerMeshVec.clear();
	//
	if (m_materialsPtrList.size() > 0) 
	{
		for (size_t i = 0; i < m_materialsPtrList.size(); ++i)
		{
			delete m_materialsPtrList[i];
		}
	}
	m_materialsPtrList.clear();
}

int Converter::ConvertSingleLTBFile(const std::string& ltbFilePath, const std::string& outFilePath)
{
	LTBModelPtr ltbModel = make_shared<LTBModel>();
	int ret = LoadLTBModel(ltbFilePath,ltbModel);
	if (ret != CONVERT_RET_OK) 
	{
		return ret;
	}
	bool success = doConvertLTB(ltbModel, outFilePath);
	if (!success)
	{
		return CONVERT_RET_EXPORT_LTB_2_FBX_FAILED;
	}
	return CONVERT_RET_OK;
}

bool Converter::doConvertLTB(LTBModelPtr ltbModel, std::string outFilePath)
{
	aiScene* exportScene = new aiScene();
	//
	grabSkeletonNodesFromLTB(ltbModel);
	//
	grabMaterialsPerMeshFromLTB(ltbModel);
	//
	grabBonesPerMeshFromLTB(ltbModel);
	//
	grabMeshesFromLTB(ltbModel);
	//
	exportScene->mRootNode = new aiNode();
	exportScene->mRootNode->mName = ltbModel->GetFilename();
	//
	const unsigned int numMeshes = m_meshesPtrVec.size();
	exportScene->mNumMeshes = numMeshes;
	exportScene->mMeshes = &m_meshesPtrVec[0];
	//
	exportScene->mMaterials = &m_materialsPtrList[0];
	exportScene->mNumMaterials = m_materialsPtrList.size();
	//
	for (size_t i = 0; i < numMeshes; ++i)
	{
		aiNode* meshSceneNode = new aiNode();
		meshSceneNode->mName = m_meshesPtrVec[i]->mName;
		meshSceneNode->mNumMeshes = 1;
		meshSceneNode->mMeshes = new unsigned int[1];
		meshSceneNode->mMeshes[0] = i;
		exportScene->mRootNode->addChildren(1, &meshSceneNode);
	}
	//
	aiNode* skeNodeArrHead = &m_skeletonNodes[0];
	exportScene->mRootNode->addChildren(1, &skeNodeArrHead);
	//
	exportScene->mNumAnimations = 0;
	exportScene->mNumCameras = 0;
	exportScene->mNumLights = 0;
	exportScene->mNumTextures = 0;
	//
	Assimp::Exporter exporter;
	aiReturn ret = exporter.Export(exportScene, "fbx", outFilePath);
	//releaseGrabbedData();
	if (ret != aiReturn_SUCCESS) 
	{
		return false;
	}
	return true;
}

void Converter::grabSkeletonNodesFromLTB(LTBModelPtr ltbModel)
{
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

void Converter::grabMaterialsPerMeshFromLTB(LTBModelPtr ltbModel)
{
	const unsigned int numMeshes = ltbModel->m_Pieces.GetSize();
	for (size_t i = 0; i < numMeshes; ++i)
	{
		aiMaterial* mat = new aiMaterial();
		m_materialsPtrList.push_back(mat);
	}
}

void Converter::grabMeshesFromLTB(LTBModelPtr ltbModel)
{
	//
	const unsigned int numMeshes = ltbModel->m_Pieces.GetSize();
	//
	for (unsigned int meshIdx = 0; meshIdx < numMeshes; ++meshIdx)
	{
		ModelPiece* piece = ltbModel->GetPiece(meshIdx);
		CDIModelDrawable* draw = piece->GetLOD(0);
		size_t numVert = draw->m_Verts.GetSize();
		//
		Mesh* mesh = new Mesh();
		mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
		mesh->mName.Set(piece->GetName());
		//
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
		mesh->mNumBones = m_bonesPtrArrPerMeshVec[meshIdx].size();
		mesh->mBones = &m_bonesPtrArrPerMeshVec[meshIdx][0];
		mesh->mNumAnimMeshes = 0;
		//
		m_meshesPtrVec.push_back(mesh);
		m_ltbDrawableList.push_back(draw);
	}
}

void Converter::grabBonesPerMeshFromLTB(LTBModelPtr ltbModel)
{
	//
	const unsigned int numMeshes = ltbModel->m_Pieces.GetSize();
	//
	for (unsigned int meshIdx = 0; meshIdx < numMeshes; ++meshIdx)
	{
		ModelPiece* piece = ltbModel->GetPiece(meshIdx);
		CDIModelDrawable* drawable = piece->GetLOD(0);
		unsigned int numVertices = drawable->m_Verts.GetSize();
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
		//构建当前网格所需的骨骼数据
		unsigned int numBonesOfMesh = bonesName.size();
		//
		BonesPtrVec bonesPtrVec;
		for (unsigned int boneIdx = 0; boneIdx < numBonesOfMesh; ++boneIdx)
		{
			string boneName = bonesName[boneIdx];
			Bone* bone = new Bone();
			bone->mName.Set(boneName.c_str());
			//
			map<string, WeightList>::iterator i = weights.find(boneName);
			if (i != weights.end())
			{
				unsigned int numWei = i->second.size();
				aiVertexWeight* weiArray = new aiVertexWeight[numWei]();
				for (size_t wIdx = 0; wIdx < numWei; ++wIdx) 
				{
					weiArray[wIdx] = i->second[wIdx];
				}
				bone->mNumWeights = numWei;
				bone->mWeights = weiArray;
			}
			//
			aiNode* sceneNode = getSkeletonNodeByName(boneName);
			if (sceneNode)
			{
				bone->mNode = sceneNode;
				bone->mArmature = sceneNode;
				//bone->mOffsetMatrix =  recuseCalcuMat(sceneNode);//sceneNode->mTransformation;//
			}
			if (!sceneNode->mMeshes) 
			{
				sceneNode->mNumMeshes = 1;
				sceneNode->mMeshes = new unsigned int[1]{ 1 };
			}
			bonesPtrVec.push_back(bone);
		}
		m_bonesPtrArrPerMeshVec.push_back(bonesPtrVec);
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

aiMatrix4x4 Converter::recuseCalcuMat(Node* sceneNode) 
{
	if (sceneNode->mParent) 
	{
		aiMatrix4x4 parent = recuseCalcuMat(sceneNode->mParent);
		return parent * sceneNode->mTransformation;
	}
	else 
	{
		return sceneNode->mTransformation;
	}
}

int Converter::LoadLTBModel(const std::string& modelFilePath, LTBModelPtr ltbModel)
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