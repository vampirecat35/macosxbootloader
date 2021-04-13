//********************************************************************
//	created:	12:11:2009   19:19
//	filename: 	LoadKernel.cpp
//	author:		tiamo
//	purpose:	load kernel
//********************************************************************

#include "StdAfx.h"

#define KERNEL_CACHE_MAGIC													0x636f6d70
#define KERNEL_CACHE_LZSS													0x6c7a7373

#if defined(__clang__) || defined(__GNUC__)
#define memset(a,b,c) __builtin_memset(a,b,c)
#endif

#if defined(__clang__) || defined(__GNUC__)
#define memcpy(a,b,c) __builtin_memcpy(a,b,c)
#endif

#if (TARGET_OS >= YOSEMITE)
	#define KERNEL_CACHE_LZVN												0x6c7a766e
#endif

//
// compressed header
//
#ifdef _MSC_VER
#include <pshpack1.h>
#endif

typedef struct _COMPRESSED_KERNEL_CACHE_HEADER
{
	//
	// signature
	//
	UINT32																	Signature;

	//
	// compress type
	//
	UINT32																	CompressType;

	//
	// adler32
	//
	UINT32																	Adler32Value;

	//
	// uncompressed size
	//
	UINT32																	UncompressedSize;

	//
	// compressed size
	//
	UINT32																	CompressedSize;

	//
	// support ASLR
	//
	UINT32																	SupportASLR;

	//
	// reserved
	//
	UINT32																	Reserved[10];

	//
	// platform name
	//
	CHAR8																	PlatformName[64];

	//
	// efi device path
	//
	CHAR8																	RootPath[256];
}COMPRESSED_KERNEL_CACHE_HEADER;

#ifdef _MSC_VER
#include <poppack.h>
#endif

//
// global
//
STATIC BOOLEAN LdrpKernelCacheOverride										= 0;
STATIC UINT64 LdrpASLRDisplacement											= 0;
STATIC CHAR8* LdrpKernelPathName											= nullptr;
STATIC EFI_DEVICE_PATH_PROTOCOL* LdrpKernelFilePath							= nullptr;
STATIC CHAR8* LdrpKernelCachePathName										= nullptr;
STATIC EFI_DEVICE_PATH_PROTOCOL* LdrpKernelCacheFilePath					= nullptr;
STATIC CHAR8* LdrpRamDiskPathName											= nullptr;
STATIC EFI_DEVICE_PATH_PROTOCOL* LdrpRamDiskFilePath						= nullptr;

//
// compute displacement
//
STATIC UINT64 LdrpComputeASLRDisplacement(UINT8 slideValue)
{
	if (!(slideValue & 0x80))
		return static_cast<UINT64>(slideValue) << 21;

	UINT32 eaxValue															= 0;
	UINT32 ebxValue															= 0;
	UINT32 ecxValue															= 0;
	UINT32 edxValue															= 0;
	ArchCpuId(1, &eaxValue, &ebxValue, &ecxValue, &edxValue);

	//
	// family, model, ext_family, ext_model
	//	6,		e,		0,			2			= Xeon MP						(45nm) -> 0x2e
	//	6,		f,		0,			2			= Xeon MP						(32nm) -> 0x2e
	//	6,		c,		0,			2			= Core i7, Xeon					(32nm) -> 0x2c
	//	6,		a,		0,			2			= 2nd Core, Xeon E3-1200		(Sand Bridge 32nm) -> 0x2a
	//	6,		d,		0,			2			= Xeon E5xx						(Sand Bridge 32nm) -> 0x2c
	//	6,		a,		0,			3			= 3nd Core, Xeon E3-1200 v2		(Sand Bridge 22nm) -> 0x3a
	//
	UINT32 family															= (eaxValue >>  8) & 0x0f;
	UINT32 model															= ((eaxValue >>  4) & 0x0e) | ((eaxValue >> 12) & 0xf0);
	return (static_cast<UINT64>(slideValue) + (family == 6 && model >= 0x2a ? 0x81 : 0x00)) << 21;
}

//
// random
//
STATIC UINT8 LdrpRandom()
{
	UINT32 eaxValue															= 0;
	UINT32 ebxValue															= 0;
	UINT32 ecxValue															= 0;
	UINT32 edxValue															= 0;
	ArchCpuId(0x80000001, &eaxValue, &ebxValue, &ecxValue, &edxValue);
	BOOLEAN supportHardwareRandom											= (ecxValue & 0x40000000) ? TRUE : FALSE;

	while(TRUE)
	{
		UINTN randomValue													= 0;
		if (supportHardwareRandom)
		{
			randomValue														= ArchHardwareRandom();
			if (randomValue)
				return static_cast<UINT8>(randomValue);
		}

		UINT64 cpuTick														= ArchGetCpuTick();
		randomValue															= (cpuTick & 0xff) ^ ((cpuTick >> 8) & 0xff);
		if (randomValue)
			return static_cast<UINT8>(randomValue);
	}
	return 0;
}

//
// check cache valid
//
STATIC EFI_STATUS LdrpKernelCacheValid(CHAR8 CONST* cachePathName, BOOLEAN* kernelCacheValid)
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE cacheFile												= {0};
	IO_FILE_HANDLE kernelFile												= {0};
	IO_FILE_HANDLE extensionsFile											= {0};
	EFI_FILE_INFO* cacheInfo												= nullptr;
	EFI_FILE_INFO* kernelInfo												= nullptr;
	EFI_FILE_INFO* extensionsInfo											= nullptr;
	EFI_FILE_INFO* checkerInfo												= nullptr;
	*kernelCacheValid														= FALSE;

#if DEBUG_LDRP_CALL_CSPRINTF
	CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(%s).\n"), cachePathName);
#endif
	__try
	{
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(1).\n"));
#endif
		//
		// open cache file
		//
		if (EFI_ERROR(IoOpenFile(cachePathName, nullptr, &cacheFile, IO_OPEN_MODE_NORMAL)))
			try_leave(LdrpKernelCachePathName ? status = EFI_NOT_FOUND : EFI_SUCCESS);
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(2).\n"));
#endif
		//
		// get cache file info
		//
		if (EFI_ERROR(status = IoGetFileInfo(&cacheFile, &cacheInfo)))
			try_leave(NOTHING);
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(3).\n"));
#endif
		//
		// check cache file info
		//
		if (!cacheInfo || (cacheInfo->Attribute & EFI_FILE_DIRECTORY))
			try_leave(status = EFI_NOT_FOUND);
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(4).\n"));
#endif
		//
		// kernel cache override
		//
		if (LdrpKernelCacheOverride)
			try_leave(*kernelCacheValid = TRUE);
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(5).\n"));
#endif
		//
		// open kernel file
		//
		if (!EFI_ERROR(IoOpenFile(LdrpKernelPathName, nullptr, &kernelFile, IO_OPEN_MODE_NORMAL)))
		{
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(6).\n"));
#endif
			//
			// get kernel file info
			//
			if (EFI_ERROR(status = IoGetFileInfo(&kernelFile, &kernelInfo)))
				try_leave(NOTHING);
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(7).\n"));
#endif
			//
			// check kernel file info
			//
			if (!kernelInfo || (kernelInfo->Attribute & EFI_FILE_DIRECTORY))
				try_leave(status = EFI_NOT_FOUND);
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(8).\n"));
#endif
			checkerInfo														= kernelInfo;
		}

		//
		// open extensions
		//
		if (!EFI_ERROR(IoOpenFile(CHAR8_CONST_STRING("System\\Library\\Extensions"), nullptr, &extensionsFile, IO_OPEN_MODE_NORMAL)))
		{
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(9).\n"));
#endif
			//
			// get extensions file info
			//
			if (EFI_ERROR(status = IoGetFileInfo(&extensionsFile, &extensionsInfo)))
				try_leave(NOTHING);
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(A).\n"));
#endif

			//
			// check extensions info
			//
			if (!extensionsInfo || !(extensionsInfo->Attribute & EFI_FILE_DIRECTORY))
				try_leave(status = EFI_NOT_FOUND);
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(B).\n"));
#endif

			//
			// get bigger
			//
			if (!checkerInfo || BlCompareTime(&checkerInfo->ModificationTime, &extensionsInfo->ModificationTime) < 0)
				checkerInfo													= extensionsInfo;
		}
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(C).\n"));
#endif

		//
		// check time
		//
		if (!checkerInfo)
			try_leave(*kernelCacheValid = TRUE);
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(D).\n"));
#endif

		//
		// add one second
		//
		EFI_TIME modifyTime													= checkerInfo->ModificationTime;
		BlAddOneSecond(&modifyTime);

		//
		// compare time
		//
		if (memcmp(&modifyTime, &cacheInfo->ModificationTime, sizeof(modifyTime)))
			status															= EFI_NOT_FOUND;
		else
		{
			*kernelCacheValid												= TRUE;
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(E).\n"));
#endif
		}
	}
	__finally
	{
		if (cacheInfo)
			MmFreePool(cacheInfo);

		if (kernelInfo)
			MmFreePool(kernelInfo);

		if (extensionsInfo)
			MmFreePool(extensionsInfo);

		IoCloseFile(&cacheFile);
		IoCloseFile(&kernelFile);
		IoCloseFile(&extensionsFile);
	}

	return status;
}

//
// setup ASLR
//
VOID LdrSetupASLR(BOOLEAN enableASLR, UINT8 slideValue)
{
	if (enableASLR)
	{
		if (!slideValue)
			slideValue														= LdrpRandom();

		BlSetBootMode(BOOT_MODE_ASLR, 0);
		LdrpASLRDisplacement												= LdrpComputeASLRDisplacement(slideValue);
	}
	else
	{
		BlSetBootMode(0, BOOT_MODE_ASLR);
		LdrpASLRDisplacement												= 0;
	}
}

//
// get aslr displacement
//
UINT64 LdrGetASLRDisplacement()
{
	return LdrpASLRDisplacement;
}

//
// get kernel path name
//
CHAR8 CONST* LdrGetKernelPathName()
{
	return LdrpKernelPathName;
}

//
// get kernel cache path name
//
CHAR8 CONST* LdrGetKernelCachePathName()
{
	return LdrpKernelCachePathName;
}

//
// get kernel cache override
//
BOOLEAN LdrGetKernelCacheOverride()
{
	return LdrpKernelCacheOverride;
}

//
// setup kernel cache path
//
VOID LdrSetupKernelCachePath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName, BOOLEAN cacheOverride)
{
	LdrpKernelCacheFilePath													= filePath;
	LdrpKernelCachePathName													= fileName;
	LdrpKernelCacheOverride													= cacheOverride;
}

//
// setup kernel path
//
VOID LdrSetupKernelPath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName)
{
	LdrpKernelFilePath														= filePath;
	LdrpKernelPathName														= fileName;
}

//
// setup ramdisk path
//
VOID LdrSetupRamDiskPath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName)
{
	LdrpRamDiskFilePath														= filePath;
	LdrpRamDiskPathName														= fileName;
}

//
// load prelinked kernel or kernel cache (fall back)
//
EFI_STATUS LdrLoadKernelCache(MACH_O_LOADED_INFO* loadedInfo, EFI_DEVICE_PATH_PROTOCOL* bootDevicePath)
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE fileHandle												= {0};
	VOID* compressedBuffer													= nullptr;
	VOID* uncompressedBuffer												= nullptr;

	__try
	{
		//
		// check mode
		//
		if (BlTestBootMode(BOOT_MODE_ALT_KERNEL))
			try_leave(status = EFI_NOT_FOUND);

		//
		// get length info
		//
		UINT8 tempBuffer[0x140]												= {0};
		CHAR8 CONST* modelName												= PeGetModelName();
		UINTN modelNameLength												= strlen(modelName);
		UINTN devicePathLength												= DevPathGetSize(bootDevicePath);
		CHAR8 CONST* fileName												= LdrpKernelPathName ? LdrpKernelPathName : LdrpKernelCachePathName;
		UINTN fileNameLength												= strlen(fileName);

		if (modelNameLength > 0x40)
			modelNameLength													= 0x40;

		if (devicePathLength > 0x100)
			devicePathLength												= 0x100;

		UINTN leftLength													= sizeof(tempBuffer) - modelNameLength - devicePathLength;

		//
		// build alder32 buffer
		//
		memcpy(tempBuffer, modelName, modelNameLength);
		memcpy(tempBuffer + 0x40, bootDevicePath, devicePathLength);
		memcpy(tempBuffer + 0x40 + devicePathLength, fileName, fileNameLength > leftLength ? leftLength : fileNameLength);

		//
		// alder32
		//
		UINT32 tempValue													= BlAdler32(tempBuffer, sizeof(tempBuffer));
		tempValue															= SWAP32(tempValue);

		//
		// build cache path
		//
		BOOLEAN netBoot														= IoBootingFromNet();
		CHAR8 kernelCachePathName[1024]										= {0};

		if (netBoot)
		{
			if (LdrpKernelCachePathName)
				strcpy(kernelCachePathName, LdrpKernelCachePathName);
		}
		else
		{
			for(UINTN i = 0; i < 5; i ++)
			{
				//
				// build path
				//
				switch(i)
				{
					case 0:
						if (LdrpKernelCachePathName)
							strcpy(kernelCachePathName, LdrpKernelCachePathName);
						break;
					case 1:
						strcpy(kernelCachePathName, (CONST CHAR8*)"System\\Library\\PrelinkedKernels\\prelinkedkernel");
						break;
					case 2:
						strcpy(kernelCachePathName, (CONST CHAR8*)"System\\Library\\Caches\\com.apple.kext.caches\\Startup\\prelinkedkernel");
						break;
					case 3:
						strcpy(kernelCachePathName, (CONST CHAR8*)"System\\Library\\Caches\\com.apple.kext.caches\\Startup\\kernelcache");
						break;
					case 4:
						kernelCachePathName[0]								= 0;
						break;
				}

				//
				// check name
				//
				if (kernelCachePathName[0])
				{
					//
					// check valid
					//
					BOOLEAN kernelCacheValid								= FALSE;

					if (EFI_ERROR(status = LdrpKernelCacheValid(kernelCachePathName, &kernelCacheValid)))
						try_leave(NOTHING);

					if (kernelCacheValid)
						break;
				}
				status														= EFI_NOT_FOUND;
			}
		}

		//
		// unable to build file path
		//
		if (EFI_ERROR(status))
			try_leave(NOTHING);

		//
		// open file
		//
		if (EFI_ERROR(status = IoOpenFile(kernelCachePathName, LdrpKernelCacheFilePath, &fileHandle, IO_OPEN_MODE_NORMAL)))
			try_leave(NOTHING);

		//
		// load as thin fat file
		//
		if (EFI_ERROR(status = MachLoadThinFatFile(&fileHandle, nullptr, nullptr)))
			try_leave(NOTHING);

		//
		// read file header
		//
		STATIC COMPRESSED_KERNEL_CACHE_HEADER fileHeader					= {0};
		UINTN readLength													= 0;

		if (EFI_ERROR(status = IoReadFile(&fileHandle, &fileHeader, sizeof(fileHeader), &readLength, FALSE)))
			try_leave(NOTHING);

		//
		// check length
		//
		if (readLength != sizeof(fileHeader))
			try_leave(status = EFI_NOT_FOUND);

		//
		// check signature
		//
		if (fileHeader.Signature == SWAP_BE32_TO_HOST(KERNEL_CACHE_MAGIC))
		{
			//
			// check compressed type
			//
			if (fileHeader.CompressType != SWAP_BE32_TO_HOST(KERNEL_CACHE_LZSS) && fileHeader.CompressType != SWAP_BE32_TO_HOST(KERNEL_CACHE_LZVN))
				try_leave(status = EFI_NOT_FOUND);

			//
			// disable ASLR
			//
			if (!fileHeader.SupportASLR)
				LdrSetupASLR(FALSE, 0);

			//
			// check platform name
			//
			if (fileHeader.PlatformName[0] && memcmp(tempBuffer, fileHeader.PlatformName, sizeof(fileHeader.PlatformName)))
				try_leave(status = EFI_NOT_FOUND);

			//
			// check root path
			//
			if (fileHeader.RootPath[0] && memcmp(tempBuffer + sizeof(fileHeader.PlatformName), fileHeader.RootPath, sizeof(fileHeader.RootPath)))
				try_leave(status = EFI_NOT_FOUND);

			//
			// allocate buffer
			//
			UINT32 compressedSize											= SWAP_BE32_TO_HOST(fileHeader.CompressedSize);
			compressedBuffer												= MmAllocatePool(compressedSize);

			if (!compressedBuffer)
				try_leave(status = EFI_OUT_OF_RESOURCES);

			//
			// read in
			//
			if (EFI_ERROR(status = IoReadFile(&fileHandle, compressedBuffer, compressedSize, &readLength, FALSE)))
				try_leave(NOTHING);

			//
			// check length
			//
			if (readLength != compressedSize)
				try_leave(status = EFI_NOT_FOUND);

			//
			// allocate buffer
			//
			UINT32 uncompressedSize											= SWAP_BE32_TO_HOST(fileHeader.UncompressedSize);
			uncompressedBuffer												= MmAllocatePool(uncompressedSize);

			if (!uncompressedBuffer)
				try_leave(status = EFI_OUT_OF_RESOURCES);

			//
			// decompress it
			//
			if (fileHeader.CompressType == SWAP_BE32_TO_HOST(KERNEL_CACHE_LZSS))
			{
#if DEBUG_LDRP_CALL_CSPRINTF
				CsPrintf(CHAR8_CONST_STRING("PIKE: Calling BlDecompressLZSS().\n"));
#endif
				if (EFI_ERROR(status = BlDecompressLZSS(compressedBuffer, compressedSize, uncompressedBuffer, uncompressedSize, &readLength)))
					try_leave(NOTHING);
			}
#if (TARGET_OS >= YOSEMITE)
			else if (fileHeader.CompressType == SWAP_BE32_TO_HOST(KERNEL_CACHE_LZVN))
			{
	#if DEBUG_LDRP_CALL_CSPRINTF
				CsPrintf(CHAR8_CONST_STRING("PIKE: Calling BlDecompressLZVN().\n"));
	#endif
				if (EFI_ERROR(status = BlDecompressLZVN(compressedBuffer, compressedSize, uncompressedBuffer, uncompressedSize, &readLength)))
				{
	#if DEBUG_LDRP_CALL_CSPRINTF
					CsPrintf(CHAR8_CONST_STRING("PIKE: BlDecompressLZVN() returned: %d!\n"), status);
	#endif
					try_leave(NOTHING);
				}
			}
#endif // #if (TARGET_OS >= YOSEMITE)

			//
			// length check
			//
			if (readLength != uncompressedSize)
			{
#if DEBUG_LDRP_CALL_CSPRINTF
				CsPrintf(CHAR8_CONST_STRING("PIKE: readLength(%d) != uncompressedSize(%d).\n"), readLength, uncompressedSize);
#endif
				try_leave(status = EFI_NOT_FOUND);
			}
			//
			// check adler32
			//
			tempValue														= BlAdler32(uncompressedBuffer, uncompressedSize);

			if (tempValue != SWAP_BE32_TO_HOST(fileHeader.Adler32Value))
			{
#if DEBUG_LDRP_CALL_CSPRINTF
				CsPrintf(CHAR8_CONST_STRING("PIKE: adler32(%d) != %d!\n"), SWAP_BE32_TO_HOST(fileHeader.Adler32Value), tempValue);
#endif
				try_leave(status = EFI_NOT_FOUND);
			}
			//
			// hack file handle
			//
			IoCloseFile(&fileHandle);
			fileHandle.EfiFileHandle										= nullptr;
			fileHandle.EfiFilePath											= nullptr;
			fileHandle.EfiLoadFileProtocol									= nullptr;
			fileHandle.FileBuffer											= static_cast<UINT8*>(uncompressedBuffer);
			fileHandle.FileOffset											= 0;
			fileHandle.FileSize												= uncompressedSize;
			uncompressedBuffer												= nullptr;
		}
		else
		{
			//
			// seek to beginning
			//
			IoSetFilePosition(&fileHandle, 0);
		}
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: Calling MachLoadMachO().\n"));
#endif
		//
		// load mach-o
		//
		if (EFI_ERROR(status = MachLoadMachO(&fileHandle, loadedInfo)))
			try_leave(NOTHING);

		DEVICE_TREE_NODE* chosenNode										= DevTreeFindNode(CHAR8_CONST_STRING("/chosen"), TRUE);

		if (chosenNode)
			DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-kernelcache-adler32"), &tempValue, sizeof(tempValue), TRUE);
	}
	__finally
	{
		if (compressedBuffer)
			MmFreePool(compressedBuffer);

		if (uncompressedBuffer)
			MmFreePool(uncompressedBuffer);

		IoCloseFile(&fileHandle);
	}
#if DEBUG_LDRP_CALL_CSPRINTF
	CsPrintf(CHAR8_CONST_STRING("PIKE: Returning from LdrLoadKernelCache(%d).\n"), status);
#endif
	return status;
}

//
// load kernel
//
EFI_STATUS LdrLoadKernel(MACH_O_LOADED_INFO* loadedInfo)
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE fileHandle												= {0};

	if (EFI_ERROR(status = IoOpenFile(LdrpKernelPathName, LdrpKernelFilePath, &fileHandle, IO_OPEN_MODE_KERNEL)))
		return status;

	status																	= MachLoadMachO(&fileHandle, loadedInfo);
	IoCloseFile(&fileHandle);

	return status;
}

//
// load ramdisk
//
EFI_STATUS LdrLoadRamDisk()
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE fileHandle												= {0};
	UINT64 physicalAddress													= 0;
	UINT64 virtualAddress													= 0;

	__try
	{
		if (EFI_ERROR(status = IoOpenFile(LdrpRamDiskPathName, LdrpRamDiskFilePath, &fileHandle, IO_OPEN_MODE_RAMDISK)))
			try_leave(NOTHING);

		UINTN bufferLength													= 0;

		if (EFI_ERROR(status = MachLoadThinFatFile(&fileHandle, nullptr, &bufferLength)))
			try_leave(NOTHING);

		UINTN allocatedLength												= bufferLength;
		physicalAddress														= MmAllocateKernelMemory(&allocatedLength, &virtualAddress);

		if (!physicalAddress)
			try_leave(status = EFI_OUT_OF_RESOURCES);

		UINTN readLength													= 0;

		if (EFI_ERROR(status = IoReadFile(&fileHandle, ArchConvertAddressToPointer(physicalAddress, VOID*), bufferLength, &readLength, FALSE)))
			try_leave(NOTHING);

		if (!EFI_ERROR(status = BlAddMemoryRangeNode(CHAR8_CONST_STRING("RAMDisk"), physicalAddress, readLength)))
			physicalAddress													= 0;
	}
	__finally
	{
		if (physicalAddress)
			MmFreeKernelMemory(virtualAddress, physicalAddress);

		IoCloseFile(&fileHandle);
	}

	return status;
}
