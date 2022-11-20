#include <windows.h>
#include "de_file.h"
#include "model.h"
#include "Converter.h"

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


int main(int argc,char** argv)
{
	if (argc <= 1) 
	{
		printf("[输入文件路径]	     [输出文件路径]\n");
		printf("[input file path]    [output file path]\n");
		printf("example：res/model.ltb res/output/model.fbx\n");
		return 0;
	}
	std::string inFile = argv[1];
	std::string inFormat = grabFileExt(inFile);
	std::string outFile;
	std::string outFormat;
	if (argc > 2) 
	{
		outFile = argv[2];
		outFormat = grabFileExt(outFile);
	}
	else 
	{
		outFile = replaceFileExt(inFile,"fbx");
		outFormat = "fbx";
	}
	printf("LTB2FBX: %s --> %s\n", inFile.c_str(), outFile.c_str());
	if (inFormat.compare("ltb") != 0)
	{
		printf("输入文件仅支持 .ltb 格式 !\n");
		printf("Input file must be .ltb format !\n");
		return 0;
	}
	if (outFormat.compare("ltb") == 0)
	{
		printf("不支持输出 .ltb 格式 !\n");
		printf("Cannot export .ltb format file !\n");
		return 0;
	}
	Converter* converter = new Converter();
	converter->SetExportFormat(outFormat);
	int ret = converter->ConvertSingleLTBFile(inFile, outFile);
	if (ret != CONVERT_RET_OK) 
	{
		if (ret == CONVERT_RET_INVALID_INPUT_FILE) 
		{
			printf("打开输入文件失败！Open input file failed ! \n");
		}
		else if(ret == CONVERT_RET_LOADING_MODEL_FAILED) 
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
    return 0;   
}