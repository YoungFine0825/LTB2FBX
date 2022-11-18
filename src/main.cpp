#include <windows.h>
#include "de_file.h"
#include "model.h"
#include "Converter.h"

std::string grabFileExt(std::string filePath) 
{
	int firstDot = filePath.find_first_of('.', 0);
	int lastDot = filePath.find_last_of('.', 0);
	int dotIndex = max(firstDot,lastDot);
	std::string ext = filePath.substr(dotIndex + 1, filePath.length() - dotIndex);
	return ext;
}

std::string replaceFileExt(std::string filePath, std::string ext)
{
	int firstDot = filePath.find_first_of('.', 0);
	int lastDot = filePath.find_last_of('.', 0);
	int dotIndex = max(firstDot, lastDot);
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
	if (!inFormat.compare("ltb")) 
	{
		printf("输入文件仅支持 .ltb 格式 !\n");
		printf("Only Support .ltb format !\n");
		return 0;
	}
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