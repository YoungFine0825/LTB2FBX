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

class Converter
{
public:
	Converter();
	~Converter();

	int ConvertSingleLTBFile(const std::string& inputFilePath, const std::string& outFilePath);
	int LoadLTBModel(const std::string& modelFilePath, Model* ltbModel);
private:
	bool readLTBHeader(LTB_Header* head, DosFileStream* stream);
	bool decodingLTBFile(const std::string& inputFilePath, DosFileStream* fileStream);
	bool doConvertLTB(Model* ltbModel, std::string outFilePath);
	void grabMeshesFromLTB(Model* ltbModel);
	void grabSkeletonNodesFromLTB(Model* ltbModel);
	void grabAndBuildMeshBones(Model* ltbModel);
	aiNode* getSkeletonNodeByName(const std::string& name);
	aiBone* getBoneByName(const std::string& name);
	//
	LzmaDecoder* m_lzmaDecoder;
	aiScene* exportScene = nullptr;
	std::vector<aiMesh*> m_meshesList;
	std::vector<CDIModelDrawable*> m_ltbDrawableList;
	std::vector<aiNode> m_skeletonNodes;
	std::map<std::string, aiNode*> m_name2SkeNode;
	std::map<std::string, int> m_name2SkeNodeIdx;
	std::vector<aiBone> m_bonesList;
	std::vector<aiBone*> m_meshBonesArray;
	std::vector<int> m_numBonesPerMesh;
};
