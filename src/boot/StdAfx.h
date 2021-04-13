//********************************************************************
//	created:	4:11:2009   10:03
//	filename: 	StdAfx.h
//	author:		tiamo
//	purpose:	stdafx
//********************************************************************

#ifndef __STDAFX_H__
#define __STDAFX_H__ 1

#ifdef __APPLE__
#define __leave
#define __try if (1)
#define __except(x) if (0 && (x))
#define __finally if (1)

#if defined(__GNU__) || defined(__clang__)
#define GNUPACK __attribute__((packed))
#else
#define GNUPACK
#endif

#ifndef nullptr
#define nullptr 0
#endif

#ifndef _INT8_T
#define _INT8_T 1
#endif
#else
#if defined(__GNU__) || defined(__clang__)
#define GNUPACK __attribute__((packed))
#else
#define GNUPACK
#endif
#endif

#include "../../sdk/include/EfiTypes.h"

#define DEBUG_LDRP_CALL_CSPRINTF											0
#define DEBUG_NVRAM_CALL_CSPRINTF											0
#define DEBUG_KERNEL_PATCHER												0

#define OS_LEGACY															6

#define YOSEMITE															10
#define EL_CAPITAN															11
#define SIERRA																12
#define HIGH_SIERRA                                                         13
#define MOJAVE                                                              14
#define CATALINA                                                            15
#define BIG_SUR                                                             16

#define TARGET_OS															SIERRA

#if (TARGET_OS >= YOSEMITE)
#ifndef LEGACY_GREY_SUPPORT
	#define LEGACY_GREY_SUPPORT												1
#endif
#else
#ifndef LEGACY_GREY_SUPPORT
	#define LEGACY_GREY_SUPPORT												0
#endif
#endif

#define PATCH_LOAD_EXECUTABLE												1
//
// grep '\x48\x89\xc3\x48\x85\xdb\x74\x71' /S*/L*/Kernels/kernel
//
#define LOAD_EXECUTABLE_TARGET_UINT64										0x487174db8548c389ULL
#define LOAD_EXECUTABLE_PATCH_UINT64										0x4812ebdb8548c389ULL

#define PATCH_READ_STARTUP_EXTENSIONS										0
//
// grep '\xe8\x25\x00\x00\x00\xeb\x05\xe8' /S*/L*/Kernels/kernel
//
#define READ_STARTUP_EXTENSIONS_TARGET_UINT64								0xe805eb00000025e8ULL
#define READ_STARTUP_EXTENSIONS_PATCH_UINT64								0xe8909000000025e8ULL

#define DO_REPLACE_BOARD_ID													1

#if DO_REPLACE_BOARD_ID
	#define MACPRO_41														"Mac-F221BEC8"
	#define MACBOOKPRO_31													"Mac-F4238BC8"

	#define DEBUG_BOARD_ID_CSPRINTF											0
#endif

#define USE_FIXED_SMBIOS_UUID												1

#define NOTHING
#define BOOTAPI																__cdecl
#define CHAR8_CONST_STRING(S)												static_cast<CHAR8 CONST*>(static_cast<VOID CONST*>(S))
#define CHAR16_CONST_STRING(S)												static_cast<CHAR16 CONST*>(static_cast<VOID CONST*>(S))
#define CHAR8_STRING(S)														static_cast<CHAR8*>(static_cast<VOID*>(S))
#define CHAR16_STRING(S)													static_cast<CHAR16*>(static_cast<VOID*>(S))
#define try_leave(S)														do{S;__leave;}while(0)
#define ARRAYSIZE(A)														(sizeof((A)) / sizeof((A)[0]))
#define ArchConvertAddressToPointer(P,T)									((T)((UINTN)(P)))
#define ArchConvertPointerToAddress(A)										((UINTN)(A))
#define ArchNeedEFI64Mode()													(MmGetKernelVirtualStart() > static_cast<UINT32>(-1) || sizeof(UINTN) == sizeof(UINT64))
#define LdrStaticVirtualToPhysical(V)										((V) & (1 * 1024 * 1024 * 1024 - 1))
#define Add2Ptr(P, O, T)													ArchConvertAddressToPointer(ArchConvertPointerToAddress(P) + (O), T)
#define PAGE_ALIGN(A)														((A) & ~EFI_PAGE_MASK)
#define BYTE_OFFSET(A)														((UINT32)((UINT64)(A)) & EFI_PAGE_MASK)

#define SWAP32(V)															((((UINT32)(V) & 0xff) << 24) | (((UINT32)(V) & 0xff00) << 8) | (((UINT32)(V) & 0xff0000) >> 8) |  (((UINT32)(V) & 0xff000000) >> 24))
#define SWAP_BE32_TO_HOST													SWAP32

#if (TARGET_OS >= EL_CAPITAN)
#ifndef kBootArgsFlagCSRActiveConfig
	#define kBootArgsFlagCSRActiveConfig	(1 << 3)	// 8
#endif

#ifndef kBootArgsFlagCSRConfigMode
	#define kBootArgsFlagCSRConfigMode		(1 << 4)	// 16
#endif

#ifndef kBootArgsFlagCSRBoot
	#define kBootArgsFlagCSRBoot			(1 << 5)	// 32
#endif

#ifndef CSR_VALID_FLAGS
	/* Rootless configuration flags */
	#define CSR_ALLOW_UNTRUSTED_KEXTS		(1 << 0)	// 1
	#define CSR_ALLOW_UNRESTRICTED_FS		(1 << 1)	// 2
	#define CSR_ALLOW_TASK_FOR_PID			(1 << 2)	// 4
	#define CSR_ALLOW_KERNEL_DEBUGGER		(1 << 3)	// 8
	#define CSR_ALLOW_APPLE_INTERNAL		(1 << 4)	// 16
	#define CSR_ALLOW_UNRESTRICTED_DTRACE	(1 << 5)	// 32
	#define CSR_ALLOW_UNRESTRICTED_NVRAM	(1 << 6)	// 64
	#define CSR_ALLOW_DEVICE_CONFIGURATION	(1 << 7)	// 128

	#define CSR_VALID_FLAGS (CSR_ALLOW_UNTRUSTED_KEXTS | \
			CSR_ALLOW_UNRESTRICTED_FS | \
			CSR_ALLOW_TASK_FOR_PID | \
			CSR_ALLOW_KERNEL_DEBUGGER | \
			CSR_ALLOW_APPLE_INTERNAL | \
			CSR_ALLOW_UNRESTRICTED_DTRACE | \
			CSR_ALLOW_UNRESTRICTED_NVRAM | \
			CSR_ALLOW_DEVICE_CONFIGURATION)
#endif
#endif // #if (TARGET_OS >= YOSMITE)

#include "../../sdk/include/EfiCommon.h"
#include "../../sdk/include/EfiApi.h"
#include "../../sdk/include/EfiImage.h"
#include "../../sdk/include/EfiDevicePath.h"
#include "../../sdk/include/IndustryStandard/Acpi.h"
#include "../../sdk/include/IndustryStandard/pci.h"
#include "../../sdk/include/IndustryStandard/Smbios.h"

#include "GuidDefine.h"
#include "RuntimeLib.h"
#include "ArchUtils.h"
#include "Memory.h"
#include "MiscUtils.h"
#include "AcpiUtils.h"
#include "PeImage.h"
#include "BootDebugger.h"
#include "Base64.h"
#include "Crc32.h"
#include "MD5.h"
#include "SHA256.h"
#include "DeviceTree.h"
#include "DevicePath.h"
#include "Config.h"
#include "FileIo.h"
#include "MachO.h"
#include "PlatformExpert.h"
#include "NetBoot.h"
#include "Hibernate.h"
#include "Console.h"
#include "Options.h"
#include "LoadKernel.h"
#include "LoadDrivers.h"
#include "BootArgs.h"
#include "MemoryMap.h"
#include "PanicDialog.h"
#include "FileVault.h"

#endif
