#pragma once

#include "bdefs.h"
#include "dtxmgr.h"
#include "load_pcx.h"
#include "pixelformat.h"
#include "streamsim.h"
#include "load_pcx.h"

#include "LzmaDecoder.h"
enum
{
	DTX_CONVERT_OK = 0,
	DTX_CONVERT_INVALID_INPUT_FILE = 1,
	DTX_CONVERT_DECODING_FAILED = 2,
	DTX_CONVERT_FAILED = 3,
};

class DtxConverter
{
public:
	DtxConverter();
	~DtxConverter();
	int ConvertSingleDTXFile(const std::string& format,const std::string& inputFilePath, const std::string& outFilePath);
	BOOL 		DTX2TGAhandler(const char* inputfile, const char* outputfile);
	void 		DTX2BPP_32Phandler(const char* inputfile, const char* outputfile);
private:
	//
	LzmaDecoder* m_lzmaDecoder;
};
