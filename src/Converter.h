#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "de_file.h"
#include "model.h"
#include "LzmaDecoder.h"
#include "ltb.h"
#include "assimp/scene.h"
enum 
{
	CONVERT_RET_OK = 0,
	CONVERT_RET_INVALID_INPUT_FILE = 1,
	CONVERT_RET_INVALID_LOAD_REQUEST = 2,
	CONVERT_RET_INVALID_HEADER = 3,
	CONVERT_RET_DECODING_FAILED = 4,
	CONVERT_RET_LOADING_MODEL_FAILED = 5,
	CONVERT_RET_EXPORT_LTB_2_FBX_FAILED = 6,
};

typedef Model LTBModel;
typedef std::shared_ptr<LTBModel> LTBModelPtr;
typedef AnimInfo LTBAnimInfo;
typedef ModelAnim LTBAnim;
//
typedef aiScene ExportScene;
typedef std::shared_ptr<ExportScene> ExportScenePtr;
//
typedef aiMesh Mesh;
typedef std::shared_ptr<Mesh> MeshPtr;
//
typedef aiNode Node;
typedef std::shared_ptr<Node> NodePtr;
//
typedef aiBone Bone;
typedef std::vector<Bone> BonesVec;
typedef std::vector<Bone*> BonesPtrVec;
typedef std::shared_ptr<Bone> BonePtr;
//
typedef aiMaterial Material;
typedef std::vector<Material*> MaterialsPtrVec;
typedef std::shared_ptr<Material> MaterialPtr;
//
typedef aiAnimation Animation;
typedef std::vector<Animation*> AnimationsPtrVec;
typedef aiNodeAnim NodeAnimation;
typedef std::vector<NodeAnimation*> NodeAnimationsPtrVec;
//

class Converter
{
public:
	Converter();
	~Converter();

	int ConvertSingleLTBFile(const std::string& inputFilePath, const std::string& outFilePath);
	int LoadLTBModel(const std::string& modelFilePath, LTBModelPtr ltbModel);
private:
	bool readLTBHeader(LTB_Header* head, DosFileStream* stream);

	bool decodingLTBFile(const std::string& inputFilePath, DosFileStream* fileStream);

	bool doConvertLTB(LTBModelPtr ltbModel, std::string outFilePath);

	void grabSkeletonNodesFromLTB(LTBModelPtr ltbModel);

	void grabAnimationsFromLTB(LTBModelPtr ltbModel);

	void grabMaterialsPerMeshFromLTB(LTBModelPtr ltbModel);

	void grabMeshesFromLTB(LTBModelPtr ltbModel);

	void grabBonesPerMeshFromLTB(LTBModelPtr ltbModel);

	aiMatrix4x4 recursCalcuMat(Node* sceneNode);

	void releaseGrabbedData();

	aiNode* getSkeletonNodeByName(const std::string& name);
	//
	LzmaDecoder* m_lzmaDecoder;
	//
	std::vector<Mesh*> m_meshesPtrVec;
	std::vector<CDIModelDrawable*> m_ltbDrawableList;
	//
	std::vector<Node> m_skeletonNodes;
	std::map<std::string, Node*> m_name2SkeNode;
	std::map<std::string, int> m_name2SkeNodeIdx;
	//
	std::vector<BonesPtrVec> m_bonesPtrArrPerMeshVec;
	//
	MaterialsPtrVec m_materialsPtrList;
	//
	AnimationsPtrVec m_animPtrList;
};
