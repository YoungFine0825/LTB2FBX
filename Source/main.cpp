#include <windows.h>
#include <filesystem>
#include "de_file.h"
#include "model.h"
#include "Converter.h"
#include "DtxConverter.h"

typedef std::vector<std::string> FilePathVec;

Converter* g_ltbConverter = new Converter();
DtxConverter* g_dtxConverter = new DtxConverter();

int findLastOf(const std::string& str,char _Ch) 
{
	int ret = -1;
	int charIndex = -1;
	for (auto& c : str)
	{
		++charIndex;
		if (c == _Ch)
		{
			ret = charIndex;
		}
	}
	return ret;
}

std::string grabFileExt(std::string filePath) 
{
	int dotIndex = findLastOf(filePath, '.');
	if (dotIndex == -1) 
	{
		return std::string("");
	}
	std::string ext = filePath.substr(dotIndex + 1, filePath.length() - dotIndex);
	for (auto& c : ext) 
		c = tolower(c);
	return ext;
}

std::string replaceFileExt(std::string filePath, std::string ext)
{
	int dotIndex = findLastOf(filePath, '.');
	std::string pathWithoutExt = filePath.substr(0, dotIndex);
	return pathWithoutExt + '.' + ext;
}

void ConvertDTX(const std::string& inputFilePath,const std::string& inputFormat,int argc, char** argv)
{
	std::string outFile;
	std::string outFormat;
	if (argc > 2)
	{
		outFile = argv[2];
		outFormat = grabFileExt(outFile);
	}
	else
	{
		outFile = replaceFileExt(inputFilePath, "tga");
		outFormat = "tga";
	}
	printf("Converting .DTX : %s -->> %s\n", inputFilePath.c_str(), outFile.c_str());
	if (outFormat != "tga") 
	{
		printf("�ܱ�Ǹ��ֻ֧�ֽ�.dtx��ʽת��Ϊ.tga��ʽ !\n");
		printf("Sorry,the output format must be \".tga\"! \n");
		return;
	}
	int ret = g_dtxConverter->ConvertSingleDTXFile(outFormat, inputFilePath, outFile);
	if (ret == 0) 
	{
		printf("ת���ɹ���Convert Successful ! \n");
	}
	else 
	{
		printf("ת��ʧ�ܣ�Convert Failed ! \n");
	}
}

void ConvertLTB(const std::string& inputFilePath, const std::string& inputFormat, int argc, char** argv)
{
	std::string outFile;
	std::string outFormat;
	if (argc > 2)
	{
		outFile = argv[2];
		outFormat = grabFileExt(outFile);
	}
	else
	{
		outFile = replaceFileExt(inputFilePath, "fbx");
		outFormat = "fbx";
	}
	printf("Converting .LTB : %s -->> %s\n", inputFilePath.c_str(), outFile.c_str());
	if (inputFormat.compare("ltb") != 0)
	{
		printf("�����ļ���֧�� .ltb ��ʽ !\n");
		printf("The input file format must be \".ltb\" !\n");
		return;
	}
	if (outFormat.compare("ltb") == 0)
	{
		printf("��֧����� .ltb ��ʽ !\n");
		printf("Cannot exporting \".ltb\" format !\n");
		return;
	}
	g_ltbConverter->SetExportFormat(outFormat);
	int ret = g_ltbConverter->ConvertSingleLTBFile(inputFilePath, outFile);
	if (ret != CONVERT_RET_OK)
	{
		if (ret == CONVERT_RET_INVALID_INPUT_FILE)
		{
			printf("�������ļ�ʧ�ܣ�Open input file failed ! \n");
		}
		else if (ret == CONVERT_RET_LOADING_MODEL_FAILED)
		{
			printf("����ģ������ʧ�ܣ�Load .ltb model failed ! \n");
		}
		else if (ret == CONVERT_RET_DECODING_FAILED)
		{
			printf("��ѹ.ltb�ļ�ʧ�ܣ�Uncompress .ltb model failed ! \n");
		}
		printf("ת��ʧ�ܣ�Convert Failed ! \n");
	}
	else
	{
		printf("ת���ɹ���Convert Successful ! \n");
	}
}

void recurseAndCollectFilePath(std::filesystem::path start, FilePathVec* filesVec)
{
	std::filesystem::directory_iterator end;
	std::filesystem::directory_iterator dirIt(start);
	for (dirIt; dirIt != end; ++dirIt)
	{
		std::filesystem::path p = *dirIt;
		//
		if (std::filesystem::is_directory(p))
		{
			recurseAndCollectFilePath(p, filesVec);
		}
		else 
		{
			std::filesystem::path ext = p.extension();
			if (ext == ".ltb" || ext == ".LTB" || ext == "dtx" || ext == ".DTX")
			{
				filesVec->push_back(p.string());
			}
		}
	}
}



int main(int argc,char** argv)
{
	if (argc <= 1) 
	{
		printf("****************************************************************\n");
		printf(" �÷�1���ڿ���̨���������\"ltb2fbx [�����ļ�·��] [����ļ�·��]\"\n");
		printf(" �÷�2�����ļ���.ltb����.dtx����Ŀ¼��ק����ִ���ļ��ϡ�\n");
		printf(" example 1��Type command line \"ltb2fbx res\\model.ltb res\\output\\model.fbx\"\n");
		printf(" example 2��Drag the file(.ltb or .dtx) or directory onto the executable file.\n");
		printf("*****************************************************************\n");
		return 0;
	}
	std::string inFile = argv[1];
	bool isDir = std::filesystem::is_directory(inFile);
	//
	FilePathVec filesVec;
	if (isDir) 
	{
		std::filesystem::path src_path(inFile);
		recurseAndCollectFilePath(src_path, &filesVec);
		if (filesVec.size() <= 0)
		{
			printf("û�п�ת�����ļ���.lbt ��.dtx�� !\n");
			printf("There is no file��.ltb or .dtx�� available to convert !\n");
			return 0;
		}
	}
	else 
	{
		filesVec.push_back(inFile);
	}
	//
	for (size_t i = 0; i < filesVec.size(); ++i) 
	{
		printf("\n\n");
		std::string inFormat = grabFileExt(filesVec[i]);
		if (inFormat == "dtx")
		{
			ConvertDTX(filesVec[i], inFormat, argc, argv);
		}
		else
		{
			ConvertLTB(filesVec[i], inFormat, argc, argv);
		}
	}
    return 0;   
}