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
		printf("很抱歉，只支持将.dtx格式转换为.tga格式 !\n");
		printf("Sorry,the output format must be \".tga\"! \n");
		return;
	}
	int ret = g_dtxConverter->ConvertSingleDTXFile(outFormat, inputFilePath, outFile);
	if (ret == 0) 
	{
		printf("转换成功！Convert Successful ! \n");
	}
	else 
	{
		printf("转换失败！Convert Failed ! \n");
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
		printf("输入文件仅支持 .ltb 格式 !\n");
		printf("The input file format must be \".ltb\" !\n");
		return;
	}
	if (outFormat.compare("ltb") == 0)
	{
		printf("不支持输出 .ltb 格式 !\n");
		printf("Cannot exporting \".ltb\" format !\n");
		return;
	}
	g_ltbConverter->SetExportFormat(outFormat);
	int ret = g_ltbConverter->ConvertSingleLTBFile(inputFilePath, outFile);
	if (ret != CONVERT_RET_OK)
	{
		if (ret == CONVERT_RET_INVALID_INPUT_FILE)
		{
			printf("打开输入文件失败！Open input file failed ! \n");
		}
		else if (ret == CONVERT_RET_LOADING_MODEL_FAILED)
		{
			printf("加载模型数据失败！Load .ltb model failed ! \n");
		}
		else if (ret == CONVERT_RET_DECODING_FAILED)
		{
			printf("解压.ltb文件失败！Uncompress .ltb model failed ! \n");
		}
		printf("转换失败！Convert Failed ! \n");
	}
	else
	{
		printf("转换成功！Convert Successful ! \n");
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
		printf(" 用法1：在控制台中输入命令：\"ltb2fbx [输入文件路径] [输出文件路径]\"\n");
		printf(" 用法2：将文件（.ltb或者.dtx）或目录拖拽到可执行文件上。\n");
		printf(" example 1：Type command line \"ltb2fbx res\\model.ltb res\\output\\model.fbx\"\n");
		printf(" example 2：Drag the file(.ltb or .dtx) or directory onto the executable file.\n");
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
			printf("没有可转换的文件（.lbt 或.dtx） !\n");
			printf("There is no file（.ltb or .dtx） available to convert !\n");
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