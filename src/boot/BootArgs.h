//********************************************************************
//	created:	8:11:2009   16:39
//	filename: 	BootArgs.h
//	author:		tiamo
//	purpose:	boot arg
//********************************************************************

#ifndef __BOOTARGS_H__
#define __BOOTARGS_H__

//
// video
//
#if defined(_MSC_VER)
#include <pshpack1.h>
#endif

typedef struct _BOOT_VIDEO_V1
{
	//
	// 32-bit VRAM base address.
	//
	UINT32																	BaseAddress;

	//
	// mode,1 = graph,2 = text
	//
	UINT32																	DisplayMode;

	//
	// bytes per row
	//
	UINT32																	BytesPerRow;

	//
	// horz res
	//
	UINT32																	HorzRes;

	//
	// vert res
	//
	UINT32																	VertRes;

	//
	// color depth
	//
	UINT32																	ColorDepth;
}BOOT_VIDEO_V1;

#if (TARGET_OS >= SIERRA)
typedef struct _BOOT_VIDEO_V2
{
	//
	// mode,1 = graph,2 = text
	//
	UINT32																	DisplayMode;
	
	//
	// bytes per row
	//
	UINT32																	BytesPerRow;
	
	//
	// horz res
	//
	UINT32																	HorzRes;
	
	//
	// vert res
	//
	UINT32																	VertRes;
	
	//
	// color depth
	//
	UINT32																	ColorDepth;

	//
	// Reserved / unused.
	//
	UINT32																	Reserved[7];

	//
	// 64-bit VRAM base address.
	//
	UINT64																	BaseAddress;
}BOOT_VIDEO_V2;
#endif

//
// boot arg
//
typedef struct _BOOT_ARGS
{
	//
	// revision
	//
	UINT16																	Revision;

	//
	// version
	//
	UINT16																	Version;

	//
	// efi mode
	//
	UINT8																	EfiMode;

	//
	// debug mode
	//
	UINT8																	DebugMode;

	//
	// flags
	//
	UINT16																	Flags;

	//
	// command line
	//
	CHAR8																	CommandLine[1024];

	//
	// memory map physical address < 4GB
	//
	UINT32																	MemoryMap;

	//
	// memory map size
	//
	UINT32																	MemoryMapSize;

	//
	// memory map descriptor size
	//
	UINT32																	MemoryMapDescriptorSize;

	//
	// memory map descriptor version
	//
	UINT32																	MemoryMapDescriptorVersion;

	//
	// Boot Video Version 1.
	//
	BOOT_VIDEO_V1															BootVideo_V1;

	//
	// device tree physical address < 4GB
	//
	UINT32																	DeviceTree;

	//
	// device tree length
	//
	UINT32																	DeviceTreeLength;

	//
	// kernel start physical address < 4GB
	//
	UINT32																	KernelAddress;

	//
	// kernel size
	//
	UINT32																	KernelSize;

	//
	// efi runtime page start physical address
	//
	UINT32																	EfiRuntimeServicesPageStart;

	//
	// efi runtime page count
	//
	UINT32																	EfiRuntimeServicesPageCount;

	//
	// efi runtime page start virtual address
	//
	UINT64																	EfiRuntimeServicesVirtualPageStart;

	//
	// system table physical address
	//
	UINT32																	EfiSystemTable;

	//
	// ASLR displacement
	//
	UINT32																	ASLRDisplacement;

	//
	// performance data start
	//
	UINT32																	PerformanceDataStart;

	//
	// performance data size
	//
	UINT32																	PerformanceDataSize;

	//
	// key store data start
	//
	UINT32																	KeyStoreDataStart;

	//
	// key store data size
	//
	UINT32																	KeyStoreDataSize;

	//
	// boot mem start
	//
	UINT64																	BootMemStart;

	//
	// boot mem size
	//
	UINT64																	BootMemSize;

	//
	// physical memory size
	//
	UINT64																	PhysicalMemorySize;

	//
	// FSB frequencey
	//
	UINT64																	FSBFrequency;

	//
	// pci config space base address
	//
	UINT64																	PCIConfigSpaceBaseAddress;

	//
	// pci config space start bus number
	//
	UINT32																	PCIConfigSpaceStartBusNumber;

	//
	// pci config space end bus number
	//
	UINT32																	PCIConfigSpaceEndBusNumber;

#if (TARGET_OS >= EL_CAPITAN)
	//
	//
	//
	UINT32																	CsrActiveConfig;

	//
	//
	//
	UINT32																	CsrCapabilities;

	//
	//
	//
	UINT32																	Boot_SMC_plimit;

	//
	//
	//
	UINT16																	BootProgressMeterStart;

	//
	//
	//
	UINT16																	BootProgressMeterEnd;
#endif

#if (TARGET_OS >= SIERRA)
	//
	// Boot Video Version 2.
	//
	BOOT_VIDEO_V2															BootVideo_V2;

	//
	// padding.
	//
	UINT32																	Reserved4[712];
#elif (TARGET_OS >= EL_CAPITAN)
	//
	// padding
	//
	UINT32																	Reserved4[726];
#else // #if (TARGET_OS >= MOUNTAIN_LION
	//
	// padding
	//
	UINT32																	Reserved4[730];
#endif
} BOOT_ARGS;

#if defined(_MSC_VER)
#include <poppack.h>
#endif

//
// add memory range
//
EFI_STATUS BlAddMemoryRangeNode(CHAR8 CONST* rangeName, UINT64 physicalAddress, UINT64 rangeLength);

//
// init boot args
//
EFI_STATUS BlInitializeBootArgs(EFI_DEVICE_PATH_PROTOCOL* bootDevicePath, EFI_DEVICE_PATH_PROTOCOL* bootFilePath, EFI_HANDLE kernelDeviceHandle, CHAR8 CONST* bootCommandLine, BOOT_ARGS** bootArgsP);

//
// finalize boot args
//
EFI_STATUS BlFinalizeBootArgs(BOOT_ARGS* bootArgs, CHAR8 CONST* kernelCommandLine, EFI_HANDLE bootDeviceHandle, struct _MACH_O_LOADED_INFO* loadedInfo);

#if (TARGET_OS >= EL_CAPITAN)
//
// Read csr-active-config from NVRAM
//
EFI_STATUS BlInitCSRState(BOOT_ARGS* bootArgs);
#endif

//
// Mimic boot.efi and set boot.efi info properties.
//
EFI_STATUS BlAddBooterInfo(DEVICE_TREE_NODE* chosenNode);

#endif /* __BOOTARGS_H__ */
