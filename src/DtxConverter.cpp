
#include "DtxConverter.h"
#include "dynarray.h"
#include "fileio.h"
#include <string>
#include "LTTexture.h"


#define DECODING_TEMP_FILE_PATH "__dtx_decoded.temp"

FormatMgr g_FormatMgr;

DtxConverter::DtxConverter() 
{
	m_lzmaDecoder = new LzmaDecoder();
}

DtxConverter::~DtxConverter() 
{
	m_lzmaDecoder->Destroy();
}

int DtxConverter::ConvertSingleDTXFile(const std::string& format, const std::string& inputFilePath, const std::string& outFilePath)
{
	FILE* f = fopen(DECODING_TEMP_FILE_PATH, "w");
	if (f)
	{
		fclose(f);
	}
	//先尝试解压缩文件
	int ret = m_lzmaDecoder->Decode(inputFilePath.c_str(), DECODING_TEMP_FILE_PATH);
	std::string realInputFilePath;
	if (ret == DEC_RET_SUCCESSFUL)
	{
		//如果解压缩成功，则将解压后的文件作为输入文件
		realInputFilePath = DECODING_TEMP_FILE_PATH;
	}
	else 
	{
		realInputFilePath = inputFilePath;
	}
	if (format == "tga") 
	{
		if (!DTX2TGAhandler(realInputFilePath.c_str(), outFilePath.c_str()))
		{
			return DTX_CONVERT_FAILED;
		}
	}
	else if (format == "bmp") 
	{
		DTX2BPP_32Phandler(realInputFilePath.c_str(), outFilePath.c_str());
	}
	return DTX_CONVERT_OK;
}

// =======================================================
// if retainHeader is true and the destination dtx exists, the header from the destination
// file is retained in the new 32P file
BOOL Write8BitDTX(CAbstractIO& file, TextureData* pData, const std::string& pFilename, bool retainHeader)
{
	BOOL retVal = FALSE;

	//! we don't support this yet
	if (pData->m_Header.GetBPPIdent() == BPP_32P)
	{
		retVal = FALSE;
	}
	else
	{
		// this is taken from WriteAsTGA it handles the conversion of any texture type
		// into a 32bit texture, through this way we only have to write the code to 
		// convert BPP_32 -> BPP_32P to support palletized textures

		CMoArray<DWORD> outputBuf;
		TextureMipData* pMip;
		DRESULT dResult;
		ConvertRequest cRequest;
		// DWORD y;
		// uint8 *pOutLine;

		pMip = &pData->m_Mips[0];
		if (!outputBuf.SetSize(pMip->m_Width * pMip->m_Height))
			return FALSE;

		cRequest.m_pSrc = (unsigned char*)pMip->m_Data;
		dtx_SetupDTXFormat(pData, cRequest.m_pSrcFormat);
		cRequest.m_SrcPitch = pMip->m_Pitch;

		cRequest.m_pDestFormat->Init(BPP_32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
		cRequest.m_pDest = (unsigned char*)outputBuf.GetArray();
		cRequest.m_DestPitch = pMip->m_Width * sizeof(DWORD);
		cRequest.m_Width = pMip->m_Width;
		cRequest.m_Height = pMip->m_Height;
		cRequest.m_Flags = 0;
		dResult = g_FormatMgr.ConvertPixels(&cRequest);

		// load the existing dtx and get its header info
		TextureData* origData = NULL;
		if (retainHeader && (LT_OK == dResult))
		{
			// open the existing dtx
			DStream* orig = streamsim_Open(pFilename.c_str(), "rb");
			if (orig)
			{
				if (dtx_Create(orig, &origData, true, true) != LT_OK)
					dResult = LT_ERROR;

				orig->Release();
			}
		}

		if (LT_OK == dResult)
		{
			LTTextureHeader texHeader(LTTextureHeader::TT_32BitTrueColor,
				1,				// num mipmaps
				pMip->m_Width,
				pMip->m_Height,
				0);			// everything right now is going to be 
								// indexed at zero, later on, each unique
								// texture will have it's own unique index
			LTTexture texture(texHeader, (uint8*)outputBuf.GetArray());
			// convert into 8 bit texture		
			LTTexture palletizedTexture;
			texture.Create8BitPalletized(palletizedTexture);
			// Create mipmaps, there's something wrong with the 4th level of the generated
			//                 mipmap, I'll look into this as soon as possible.
			palletizedTexture.CreateMipMaps(3);

			uint32 allocSize, textureDataSize;
			TextureData* pTextureData = dtx_Alloc(BPP_32P,
				palletizedTexture.GetTextureHeader()->GetWidth(),
				palletizedTexture.GetTextureHeader()->GetHeight(),
				palletizedTexture.GetTextureHeader()->GetNumMipMaps(),
				&allocSize, &textureDataSize);
			if (pTextureData)
			{
				// copy applicable header info from the original texture
				// (doesn't copy any sections)
				if (retainHeader && origData)
				{
					DtxHeader* orig = &origData->m_Header;
					DtxHeader* cur = &pTextureData->m_Header;

					// copy over flags that apply to 32P textures
					cur->m_IFlags = 0;
					if (orig->m_IFlags & DTX_FULLBRITE)
						cur->m_IFlags |= DTX_FULLBRITE;
					if (orig->m_IFlags & DTX_SECTIONSFIXED)
						cur->m_IFlags |= DTX_SECTIONSFIXED;
					if (orig->m_IFlags & DTX_32BITSYSCOPY)
						cur->m_IFlags |= DTX_32BITSYSCOPY;

					cur->m_UserFlags = orig->m_UserFlags;

					for (int i = 0; i < 12; i++)
					{
						// skip over BPPIdent
						if (i != 2)
							cur->m_Extra[i] = orig->m_Extra[i];
					}

					strcpy(cur->m_CommandString, orig->m_CommandString);
				}

				// now let's copy our pixel data into the mip data
				uint32 sourceMipDataOffset = (256 * 4);
				uint32 w = palletizedTexture.GetTextureHeader()->GetWidth();
				uint32 h = palletizedTexture.GetTextureHeader()->GetHeight();

				for (int iMipmap = 0; iMipmap < pTextureData->m_Header.m_nMipmaps; iMipmap++)
				{
					TextureMipData* pMip = &pTextureData->m_Mips[iMipmap];

					memcpy(pMip->m_Data,
						palletizedTexture.GetTextureData() + sourceMipDataOffset, // copy after the start of the pallete
						w * h);
					pMip->m_Pitch = w;
					pMip->m_Height = h;
					pMip->m_Width = w;

					// PrintTextureMipDataInfo(pMip);

					sourceMipDataOffset = sourceMipDataOffset + (w * h);
					w = w / 2;
					h = h / 2;
				}
				pTextureData->m_Header.m_Extra[1] = (uint8)pTextureData->m_Header.m_nMipmaps;

				// now let's create the pallete section
				DtxSection* pPalleteSection = (DtxSection*)malloc(sizeof(SectionHeader) +
					sizeof(DtxSection*) +
					((sizeof(uint8) * 4) * 256));
				if (pPalleteSection)
				{
					strcpy(pPalleteSection->m_Header.m_Type, "PALLETE32");
					strcpy(pPalleteSection->m_Header.m_Name, "");
					memcpy(pPalleteSection->m_Data, palletizedTexture.GetTextureData(), ((sizeof(uint8) * 4) * 256));
					pPalleteSection->m_Header.m_DataLen = ((sizeof(uint8) * 4) * 256);

					pPalleteSection->m_pNext = LTNULL;
					// now let's tell pTextureData about our Pallete Section
					pTextureData->m_pSections = pPalleteSection;
					pTextureData->m_Header.m_IFlags |= DTX_SECTIONSFIXED;

					ILTStream* pStream = streamsim_Open((const char*)(pFilename.c_str()), "wb");
					if (pStream)
					{
						// now save the dtx into a file
						if (LT_OK == dtx_Save(pTextureData, pStream))
						{
							pStream->Release();
							retVal = TRUE;
						}
					}
				}

				dtx_Destroy(pTextureData);
				pTextureData = NULL;
			}

			if (origData)
				dtx_Destroy(origData);
		}
	}
	return retVal;
}

BOOL WriteTga(CAbstractIO& file, TextureData* pData)
{
	CMoArray<DWORD> outputBuf;
	TextureMipData* pMip;
	DRESULT dResult;
	ConvertRequest cRequest;
	TGAHeader hdr;
	DWORD y, * pOutLine;
	BOOL retVal = FALSE;
	//! we don't support this yet
	if (pData->m_Header.GetBPPIdent() == BPP_32P)
	{
		retVal = FALSE;
	}
	else
	{
		pMip = &pData->m_Mips[0];

		if (!outputBuf.SetSize(pMip->m_Width * pMip->m_Height))
		{
			retVal = FALSE;
		}
		else
		{
			cRequest.m_pSrc = (unsigned char*)pMip->m_Data;
			dtx_SetupDTXFormat(pData, cRequest.m_pSrcFormat);
			cRequest.m_SrcPitch = pMip->m_Pitch;


			cRequest.m_pDestFormat->Init(BPP_32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
			cRequest.m_pDest = (unsigned char*)outputBuf.GetArray();
			cRequest.m_DestPitch = pMip->m_Width * sizeof(DWORD);
			cRequest.m_Width = pMip->m_Width;
			cRequest.m_Height = pMip->m_Height;
			cRequest.m_Flags = 0;
			dResult = g_FormatMgr.ConvertPixels(&cRequest);

			// Write it all out.
			memset(&hdr, 0, sizeof(hdr));
			hdr.m_Width = (unsigned short)pMip->m_Width;
			hdr.m_Height = (unsigned short)pMip->m_Height;
			hdr.m_PixelDepth = 32;
			hdr.m_ImageType = 2;
			file.Write(&hdr, sizeof(hdr));

			for (y = 0; y < pMip->m_Height; y++)
			{
				pOutLine = &outputBuf[(pMip->m_Height - y - 1) * pMip->m_Width];
				file.Write(pOutLine, sizeof(DWORD) * pMip->m_Width);
			}
			retVal = TRUE;
		}
	}
	return retVal;
}

// =====================================================================
BOOL SaveDtxAsTga(DStream* pDtxFile, CAbstractIO& outFile)
{
	TextureData* pData;
	BOOL bRet;


	if (dtx_Create(pDtxFile, &pData, FALSE) != DE_OK)
		return FALSE;

	bRet = WriteTga(outFile, pData);
	dtx_Destroy(pData);
	return bRet;
}

// outFile appears to be unused
BOOL SaveDtxAs8Bit(DStream* pDtxFile, CAbstractIO& outFile, const std::string& pFilename, bool retainHeader)
{
	TextureData* pData;
	BOOL bRet = TRUE;

	if (dtx_Create(pDtxFile, &pData, FALSE) != DE_OK)
		return FALSE;

	bRet = Write8BitDTX(outFile, pData, pFilename.c_str(), retainHeader);
	dtx_Destroy(pData);
	return bRet;
}

BOOL DtxConverter::DTX2TGAhandler(const char* inputfile, const char* outputfile)
{
	CMoFileIO	outFile;
	DStream* pStream;

	if (!outFile.Open(outputfile, "wb"))
	{
		printf("\nError: can't open %s", outputfile);
		return FALSE;
	}

	if (!(pStream = streamsim_Open((const char*)inputfile, "rb")))
	{
		printf("\nError: can't open %s", inputfile);
		outFile.Close();
		return FALSE;
	}
	if (!SaveDtxAsTga(pStream, outFile))
	{
		printf("\nError: operation unsuccessful");
		return FALSE;
	}
	pStream->Release();
	return TRUE;
}

void DtxConverter::DTX2BPP_32Phandler(const char* inputfile, const char* outputfile)
{
	CMoFileIO	outFile;
	DStream* pStream;

	if (!outFile.Open(outputfile, "wb"))
	{
		printf("\nError: can't open %s", outputfile);
		return;
	}

	if (!(pStream = streamsim_Open((const char*)inputfile, "rb")))
	{
		printf("\nError: can't open %s", inputfile);
		outFile.Close();
		return;
	}
	std::string dtxfilename = outputfile;
	// _asm int 3;
	if (!SaveDtxAs8Bit(pStream, outFile, dtxfilename,false))
	{
		printf("\nError: operation unsuccessful");
	}
	pStream->Release();
}