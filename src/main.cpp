#include <windows.h>
#include "de_file.h"
#include "model.h"
#include "Converter.h"
#include "DtxConverter.h"

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
	int ret = DtxConverter::ConvertSingleDTXFile(outFormat, inputFilePath, outFile);
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
	Converter* converter = new Converter();
	converter->SetExportFormat(outFormat);
	int ret = converter->ConvertSingleLTBFile(inputFilePath, outFile);
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


int main(int argc,char** argv)
{
	if (argc <= 1) 
	{
		printf("****************************************************************\n");
		printf(" 用法1：在控制台中输入命令：\"ltb2fbx [输入文件路径]\"\n");
		printf(" 用法2：在控制台中输入命令：\"ltb2fbx [输入文件路径] [输出文件路径]\"\n");
		printf(" 用法3：将文件（.ltb或者.dtx）拖拽到可执行文件上。\n");
		printf(" example 1：Type command line \"ltb2fbx res\\model.ltb\"	\n");
		printf(" example 2：Type command line \"ltb2fbx res\\model.ltb res\\output\\model.fbx\"\n");
		printf(" example 3：Drag the file(.ltb or .dtx) onto the executable file.\n");
		printf("*****************************************************************\n");
		return 0;
	}
	std::string inFile = argv[1];
	std::string inFormat = grabFileExt(inFile);
	if (inFormat == "dtx") 
	{
		ConvertDTX(inFile,inFormat,argc,argv);
	}
	else 
	{
		ConvertLTB(inFile, inFormat, argc, argv);
	}
    return 0;   
}