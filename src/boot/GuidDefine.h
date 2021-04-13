//********************************************************************
//	created:	4:8:2012   13:39
//	filename: 	GuidDefine.h
//	author:		tiamo
//	purpose:	guid define
//********************************************************************

#ifndef __GUIDDEFINE_H__
#define __GUIDDEFINE_H__


#include "../../sdk/include/EfiCommon.h"

#include "../../sdk/include/Guid/AppleAcpiVariable/AppleAcpiVariable.h"
#include "../../sdk/include/Guid/Smbios/SmBios.h"
#include "../../sdk/include/Guid/AppleNVRAMVariable/AppleNVRAMVariable.h"
#include "../../sdk/include/Guid/AppleFirmwareVariable/AppleFirmwareVariable.h"
#include "../../sdk/include/Guid/AppleAcpiVariable/AppleAcpiVariable.h"
#include "../../sdk/include/Guid/AppleFirmwareFileName/AppleFirmwareFileName.h"
#include "../../sdk/include/Guid/AppleRamDmgDevicePath/AppleRamDmgDevicePath.h"
#include "../../sdk/include/Guid/AppleFileVaultVariable/AppleFileVaultVariable.h"

#include "../../sdk/include/Protocol/LoadedImage/LoadedImage.h"
#include "../../sdk/include/Protocol/PciIo/PciIo.h"
#include "../../sdk/include/Protocol/DataHub/DataHub.h"
#include "../../sdk/include/Protocol/DevicePath/DevicePath.h"
#include "../../sdk/include/Protocol/ConsoleControl/ConsoleControl.h"
#include "../../sdk/include/Protocol/SimpleTextOut/SimpleTextOut.h"
#include "../../sdk/include/Protocol/SimpleFileSystem/SimpleFileSystem.h"
#include "../../sdk/include/Protocol/FileInfo/FileInfo.h"
#include "../../sdk/include/Protocol/LoadFile/LoadFile.h"
#include "../../sdk/include/Protocol/BlockIO/BlockIO.h"
#include "../../sdk/include/Protocol/DiskIO/DiskIo.h"
#include "../../sdk/include/Protocol/GraphicsOutput/GraphicsOutput.h"
#include "../../sdk/include/Protocol/UgaDraw/UgaDraw.h"
#include "../../sdk/include/Protocol/FirmwareVolume/FirmwareVolume.h"
#include "../../sdk/include/Protocol/FirmwareVolumeDispatch/FirmwareVolumeDispatch.h"
#include "../../sdk/include/Protocol/SimplePointer/SimplePointer.h"
#include "../../sdk/include/Protocol/AppleKeyState/AppleKeyState.h"
#include "../../sdk/include/Protocol/AppleSystemInfo/AppleSystemInfo.h"
#include "../../sdk/include/Protocol/AppleDeviceProperty/AppleDeviceProperty.h"
#include "../../sdk/include/Protocol/AppleNetBoot/AppleNetBoot.h"
#include "../../sdk/include/Protocol/AppleFirmwarePassword/AppleFirmwarePassword.h"
#include "../../sdk/include/Protocol/AppleDeviceControl/AppleDeviceControl.h"
#include "../../sdk/include/Protocol/AppleGraphConfig/AppleGraphConfig.h"
#include "../../sdk/include/Protocol/AppleGraphInfo/AppleGraphInfo.h"
#include "../../sdk/include/Protocol/AppleImageCodec/AppleImageCodec.h"
#include "../../sdk/include/Protocol/AppleDiskIo/AppleDiskIo.h"
#include "../../sdk/include/Protocol/AppleSMC/AppleSMC.h"

extern EFI_SYSTEM_TABLE*													EfiSystemTable;
extern EFI_BOOT_SERVICES*													EfiBootServices;
extern EFI_RUNTIME_SERVICES*												EfiRuntimeServices;
extern EFI_HANDLE															EfiImageHandle;

extern EFI_GUID																EfiDataHubProtocolGuid;
extern EFI_GUID																EfiAcpi20TableGuid;
extern EFI_GUID																EfiAcpiTableGuid;
extern EFI_GUID																EfiLoadedImageProtocolGuid;
extern EFI_GUID																EfiPciIoProtocolGuid;
extern EFI_GUID																EfiSmbiosTableGuid;
extern EFI_GUID																EfiConsoleControlProtocolGuid;
extern EFI_GUID																EfiSimpleFileSystemProtocolGuid;
extern EFI_GUID																EfiDevicePathProtocolGuid;
extern EFI_GUID																EfiBlockIoProtocolGuid;
extern EFI_GUID																EfiDiskIoProtocolGuid;
extern EFI_GUID																EfiGraphicsOutputProtocolGuid;
extern EFI_GUID																EfiUgaDrawProtocolGuid;
extern EFI_GUID																EfiFileInfoGuid;
extern EFI_GUID																EfiLoadFileProtocolGuid;
extern EFI_GUID																EfiFirmwareVolumeProtocolGuid;
extern EFI_GUID																EfiFirmwareVolumeDispatchProtocolGuid;
extern EFI_GUID																EfiDevicePathMessagingSASGuid;
extern EFI_GUID																EfiSimplePointerProtocolGuid;
extern EFI_GUID																AppleKeyStateProtocolGuid;
extern EFI_GUID																AppleNVRAMVariableGuid;
extern EFI_GUID																AppleFirmwareVariableGuid;
extern EFI_GUID																AppleAcpiVariableGuid;
extern EFI_GUID																AppleSystemInfoProducerNameGuid;
extern EFI_GUID																AppleDevicePropertyProtocolGuid;
extern EFI_GUID																AppleNetBootProtocolGuid;
extern EFI_GUID																ApplePasswordUIEfiFileNameGuid;
extern EFI_GUID																AppleFirmwarePasswordProtocolGuid;
extern EFI_GUID																AppleDeviceControlProtocolGuid;
extern EFI_GUID																AppleGraphConfigProtocolGuid;
extern EFI_GUID																AppleGraphInfoProtocolGuid;
extern EFI_GUID																AppleRamDmgDevicePathGuid;
extern EFI_GUID																AppleImageCodecProtocolGuid;
extern EFI_GUID																AppleDiskIoProtocolGuid;
extern EFI_GUID																AppleSMCProtocolGuid;
extern EFI_GUID																AppleFileVaultVariableGuid;

BOOLEAN isEfiNullGuid(EFI_GUID* aGuid);

#endif /* __GUIDDEFINE_H__ */
