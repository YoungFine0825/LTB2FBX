#include <windows.h>
#include "de_file.h"
#include "model.h"
#include "Converter.h"


int main(int argc,char** argv)
{
	if (argc <= 1) 
	{
		printf("[input-file] [out-file]");
		return 0;
	}
	std::string inFile = argv[1];
	std::string outFile = argv[2];
	Converter* converter = new Converter();
	int ret = converter->ConvertSingleLTBFile(inFile, outFile);
	if (ret != CONVERT_RET_OK) 
	{
		printf("Convert Failed ! file= %s\n", inFile.c_str());
	}
	else 
	{
		printf("Convert Successful ! file= %s\n",outFile.c_str());
	}
    return 0;   
}