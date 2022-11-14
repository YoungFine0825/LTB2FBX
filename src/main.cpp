#include <windows.h>
#include "de_file.h"
#include "model.h"
#include "Converter.h"


int main(int argc,char** argv)
{
	std::string inFile = "bin/res/Yunyoyo/src/Esports_Yun_BODY_GR.ltb";
	//char inFile[] = "bin/res/CFAnim/M-MOTION_BLUE.ltb";
	std::string outFile = "bin/res/Esports_Yun_BODY_GR.fbx";
	Converter* converter = new Converter();
	int ret = converter->ConvertSingleLTBFile(inFile, outFile);

    return 0;   
}