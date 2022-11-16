
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
	m_meshesPtrVec.clear();
	//
	m_name2SkeNode.clear();
	m_name2SkeNodeIdx.clear();
	m_skeletonNodes.clear();
	//
	m_bonesPtrArrPerMeshVec.clear();
	//
	m_materialsPtrList.clear();
	//
	m_nodeAnimPtrVecList.clear();
	m_animPtrVec.clear();
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
	//releaseGrabbedData();
	if (!success)
	{
		return CONVERT_RET_EXPORT_LTB_2_FBX_FAILED;
	}
	return CONVERT_RET_OK;
}

bool Converter::doConvertLTB(LTBModelPtr ltbModel, std::string outFilePath)
{
	ExportScene* exportScene = new ExportScene();
	//
	grabSkeletonNodesFromLTB(ltbModel);
	//
	grabMaterialsPerMeshFromLTB(ltbModel);
	//
	grabBonesPerMeshFromLTB(ltbModel);
	//
	grabAnimationsFromLTB(ltbModel);
	//
	grabMeshesFromLTB(ltbModel);
	//
	//exportScene->mFlags = AI_SCENE_FLAGS_ALLOW_SHARED;
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
	exportScene->mNumAnimations = m_animPtrVec.size();
	exportScene->mAnimations = &m_animPtrVec[0];
	//
	exportScene->mNumCameras = 0;
	exportScene->mNumLights = 0;
	exportScene->mNumTextures = 0;
	//
	printExportOverview(exportScene, ltbModel);
	//
	Assimp::Exporter exporter;
	aiReturn ret = exporter.Export(exportScene, m_exportFormat, outFilePath);
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

void Converter::grabAnimationsFromLTB(LTBModelPtr ltbModel) 
{
	int numAnimInfo = ltbModel->m_Anims.GetSize();
	if (numAnimInfo <= 0) 
	{
		return;
	}
	if (m_maxNumOutputAnim > -1) 
	{
		numAnimInfo = min(numAnimInfo, m_maxNumOutputAnim);
	}
	//
	unsigned int numSkeNodes = ltbModel->NumNodes();
	m_nodeAnimPtrVecList.resize(numAnimInfo);
	double toSecond = 1000;
	for (size_t animInfoIdx = 0; animInfoIdx < numAnimInfo; ++animInfoIdx)
	{
		LTBAnim* ltbAnim = ltbModel->GetAnim(animInfoIdx);
		string ltbAnimName = ltbAnim->GetName();
		unsigned int ltbAnimDuration = ltbAnim->GetAnimTime();
		unsigned int numLTBKeyFrame = ltbAnim->m_KeyFrames.GetSize();
		//
		Animation* anim = new Animation();
		anim->mName = ltbAnimName;
		anim->mDuration = (double)ltbAnimDuration / toSecond;
		anim->mTicksPerSecond = 0;
		NodeAnimationsPtrVec* nodeAnimPtrVec = &m_nodeAnimPtrVecList[animInfoIdx];
		//
		for (size_t skeNodeIdx = 0; skeNodeIdx < numSkeNodes; ++skeNodeIdx)
		{
			NodeAnimation* nodeAnim = new NodeAnimation();
			nodeAnim->mNodeName = ltbModel->GetNode(skeNodeIdx)->GetName();
			nodeAnim->mNumPositionKeys = numLTBKeyFrame;
			nodeAnim->mPositionKeys = new aiVectorKey[numLTBKeyFrame]();
			nodeAnim->mNumRotationKeys = numLTBKeyFrame;
			nodeAnim->mRotationKeys = new aiQuatKey[numLTBKeyFrame]();
			nodeAnim->mNumScalingKeys = 0;
			nodeAnim->mPreState = aiAnimBehaviour_LINEAR;
			nodeAnim->mPostState = aiAnimBehaviour_LINEAR;
			//
			LTBAnimNode* ltbAnimNode = ltbAnim->GetAnimNode(skeNodeIdx);
			//printf("Anim Node %s\n", nodeAnim->mNodeName.data);
			//
			for (unsigned int k = 0; k < numLTBKeyFrame; ++k)
			{
				LTBAnimKeyFrame frame = ltbAnim->m_KeyFrames[k];
				LTVector pos;
				LTRotation quat;
				ltbAnimNode->GetData(k, pos, quat);
				//
				double frameTime = (double)frame.m_Time / toSecond;
				aiVector3D nodePos(pos.x, pos.y, pos.z);
				aiQuaternion nodeRot(quat.m_Quat[3], quat.m_Quat[0], quat.m_Quat[1], quat.m_Quat[2]);
				nodeAnim->mPositionKeys[k].mTime = frameTime;
				nodeAnim->mPositionKeys[k].mValue = nodePos;
				//
				nodeAnim->mRotationKeys[k].mTime = frameTime;
				nodeAnim->mRotationKeys[k].mValue = nodeRot;
				//
				//printf("   KeyFrame%d  x=%f y=%f z=%f \n", k, nodePos.x, nodePos.y, nodePos.z);
				//printf("   KeyFrame%d  w=%f x=%f y=%f z=%f \n", k, nodeRot.w, nodeRot.x, nodeRot.y, nodeRot.z);
			}
			//
			nodeAnimPtrVec->push_back(nodeAnim);
		}
		//
		anim->mChannels = &(*nodeAnimPtrVec)[0];
		anim->mNumChannels = nodeAnimPtrVec->size();
		anim->mNumMeshChannels = 0;
		anim->mNumMorphMeshChannels = 0;
		//
		m_animPtrVec.push_back(anim);
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
	map<string, unsigned int> createdBones;
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
					weiArray[wIdx].mVertexId = i->second[wIdx].mVertexId;
					weiArray[wIdx].mWeight = i->second[wIdx].mWeight;
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
				bone->mOffsetMatrix = sceneNode->mTransformation;//recursCalcuMat(sceneNode);//
			}
			bonesPtrVec.push_back(bone);
			//
			createdBones[boneName] = meshIdx;
		}
		m_bonesPtrArrPerMeshVec.push_back(bonesPtrVec);
	}
	//
	size_t numSkeNodes = m_skeletonNodes.size();
	for (size_t si = 0; si < numSkeNodes; ++si) 
	{
		Node* skeNode = &m_skeletonNodes[si];
		map<string, unsigned int>::iterator it = createdBones.find(skeNode->mName.data);
		if (it == createdBones.end()) 
		{
			Node* parentNode = skeNode->mParent;
			if (parentNode) 
			{
				map<string, unsigned int>::iterator parnetNodeIt = createdBones.find(parentNode->mName.data);
				if (parnetNodeIt != createdBones.end())
				{
					unsigned int meshIdx = parnetNodeIt->second;
					Bone* bone = new Bone();
					bone->mName.Set(skeNode->mName.data);
					aiNode* sceneNode = getSkeletonNodeByName(skeNode->mName.data);
					if (sceneNode)
					{
						bone->mNode = sceneNode;
						bone->mArmature = sceneNode;
						bone->mOffsetMatrix = sceneNode->mTransformation;//recursCalcuMat(sceneNode);//
					}
					m_bonesPtrArrPerMeshVec[meshIdx].push_back(bone);
				}
			}
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

aiMatrix4x4 Converter::recursCalcuMat(Node* sceneNode) 
{
	if (sceneNode->mParent) 
	{
		aiMatrix4x4 parent = recursCalcuMat(sceneNode->mParent);
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

void Converter::SetExportFormat(string formatExt) 
{
	m_exportFormat = formatExt;
}

void Converter::printExportOverview(ExportScene* exportScene,LTBModelPtr ltbModel)
{
	printf("-*- -*- -*- 摘要 -*- -*- -*-\n");
	printf("* 导出格式：%s\n", m_exportFormat.c_str());
	size_t numMeshes = m_meshesPtrVec.size();
	printf("* 网格数量：%d\n", numMeshes);
	for (size_t mi = 0; mi < numMeshes; ++mi)
	{
		printf("*     网格名称：%s  顶点数量：%d  三角面数量：%d\n", m_meshesPtrVec[mi]->mName.data, m_meshesPtrVec[mi]->mNumVertices, m_meshesPtrVec[mi]->mNumFaces);
	}
	printf("* 骨骼数量：%d\n", m_skeletonNodes.size());
	printf("* 动画数量：%d\n", m_animPtrVec.size());
	printf("-*- -*- -*- -*- -*- -*- -*-\n");
	printf("* 开始转换... Processing...\n");
	if (m_animPtrVec.size() > 30) 
	{
		printf("-*- -*- Warning -*- -*-\n"); 
		printf("* 动画数量较多，导出会比较耗时，请耐心等待...\n");
		printf("* There are too many animations，please waiting for a seconds...\n");
		printf("-*- -*- -*- -*- -*- -*-\n");
	}
}