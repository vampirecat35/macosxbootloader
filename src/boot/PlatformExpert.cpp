//********************************************************************
//	created:	7:11:2009   12:50
//	filename: 	PlatformExpert.cpp
//	author:		tiamo
//	purpose:	platform expert
//********************************************************************

#include "StdAfx.h"

#if defined(__clang__) || defined(__GNUC__)
#define memcpy(a,b,c) __builtin_memcpy(a,b,c)
#endif

#if defined(__clang__) || defined(__GNUC__)
#define memset(a,b,c) __builtin_memset(a,b,c)
#endif

//
// global
//
STATIC CHAR8 PepModelName[0x41]												= {0};


//
// init platform expert
//
EFI_STATUS PeInitialize()
{
	DEVICE_TREE_NODE* platformNode											= DevTreeFindNode(CHAR8_CONST_STRING("/efi/platform"), TRUE);

	if(!platformNode)
		return EFI_OUT_OF_RESOURCES;

#if USE_FIXED_SMBIOS_UUID
	//
	// Replace the value with a randomly generated UUID.
	//
	STATIC CHAR8 mySmbiosUuid[]												= { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };

	DevTreeAddProperty(platformNode, CHAR8_CONST_STRING("system-id"), (CHAR8*)&mySmbiosUuid, 16, TRUE);
#else
	EFI_GUID smbiosUuid														= BlGetSmbiosUuid();

	DevTreeAddProperty(platformNode, CHAR8_CONST_STRING("system-id"), (CHAR8*)&smbiosUuid, 16, TRUE);
#endif

	EFI_DATA_HUB_PROTOCOL* dataHubProtocol									= nullptr;
	EFI_STATUS status														= EfiBootServices->LocateProtocol(&EfiDataHubProtocolGuid, nullptr, reinterpret_cast<VOID**>(&dataHubProtocol));

	if(EFI_ERROR(status))
		return EFI_SUCCESS;

	UINT64 monotonicCount													= 0;

	do
	{
		EFI_DATA_RECORD_HEADER* recordHeader								= nullptr;
		status																= dataHubProtocol->GetNextRecord(dataHubProtocol, &monotonicCount, 0, &recordHeader);

		if(EFI_ERROR(status))
			break;

		//
		// SystemSerialNumber + FSBFrequency + Model + DevicePathsSupported
		//
		if(!memcmp(&recordHeader->ProducerName, &AppleSystemInfoProducerNameGuid, sizeof(AppleSystemInfoProducerNameGuid)))
		{
			APPLE_SYSTEM_INFO_DATA_RECORD* dataRecord						= static_cast<APPLE_SYSTEM_INFO_DATA_RECORD*>(static_cast<VOID*>(recordHeader + 1));
			UINTN nameLength												= (dataRecord->NameLength / sizeof(CHAR16) * 3 + 1) * sizeof(CHAR8);
			CHAR8* utf8NameBuffer											= static_cast<CHAR8*>(MmAllocatePool(nameLength));

			if(!utf8NameBuffer)
				break;

			if(!EFI_ERROR(BlUnicodeToUtf8(Add2Ptr(dataRecord, sizeof(APPLE_SYSTEM_INFO_DATA_RECORD), CHAR16*), dataRecord->NameLength / sizeof(CHAR16), utf8NameBuffer, nameLength)))
				DevTreeAddProperty(platformNode, utf8NameBuffer, Add2Ptr(dataRecord, sizeof(APPLE_SYSTEM_INFO_DATA_RECORD) + dataRecord->NameLength, VOID*), dataRecord->ValueLength, TRUE);

			MmFreePool(utf8NameBuffer);
		}
	}while(monotonicCount);

	return EFI_SUCCESS;
}

//
// get model name
//
CHAR8 CONST* PeGetModelName()
{
	__try
	{
		if(PepModelName[0])
			try_leave(NOTHING);

		DEVICE_TREE_NODE* platformNode										= DevTreeFindNode(CHAR8_CONST_STRING("/efi/platform"), FALSE);

		if(platformNode)
		{
			UINT32 nameLength												= 0;
			CHAR16 CONST* unicodeModelName									= static_cast<CHAR16 CONST*>(DevTreeGetProperty(platformNode, CHAR8_CONST_STRING("Model"), &nameLength));
			if(unicodeModelName && nameLength)
			{
				nameLength													/= sizeof(CHAR16);
				if(!unicodeModelName[nameLength - 1])
					nameLength												-= 1;

				BlUnicodeToUtf8(unicodeModelName, nameLength, PepModelName, ARRAYSIZE(PepModelName) - 1);
			}
			else
			{
				EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE CONST* acpiFadt	= static_cast<EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE CONST*>(AcpiGetFixedAcpiDescriptionTable());

				if(acpiFadt)
				{
					EFI_ACPI_DESCRIPTION_HEADER* acpiDsdt					= nullptr;

					if(acpiFadt->Header.Revision >= EFI_ACPI_3_0_REVISION)
						acpiDsdt											= ArchConvertAddressToPointer(acpiFadt->XDsdt, EFI_ACPI_DESCRIPTION_HEADER*);
					else
						acpiDsdt											= ArchConvertAddressToPointer(acpiFadt->Dsdt, EFI_ACPI_DESCRIPTION_HEADER*);

					if(acpiDsdt)
					{
						CHAR8 tempBuffer[sizeof(acpiDsdt->OemTableId) + 1]	= {0};
						memcpy(tempBuffer, &acpiDsdt->OemTableId, sizeof(acpiDsdt->OemTableId));

						CHAR8 CONST* oemTableId								= tempBuffer;

						while(*oemTableId == ' ')
							oemTableId										+= 1;

						snprintf(PepModelName, ARRAYSIZE(PepModelName) - 1, CHAR8_CONST_STRING("%s%d,%d"), oemTableId, acpiDsdt->OemRevision >> 16, acpiDsdt->OemRevision & 0xffff);
					}
				}
			}
		}
	}
	__finally
	{
		if(!PepModelName[0])
			strcpy(PepModelName, CHAR8_CONST_STRING("ACPI"));
	}

	return PepModelName;
}

//
// setup device tree
//
EFI_STATUS PeSetupDeviceTree()
{
	EFI_STATUS status														= EFI_SUCCESS;
	VOID* deviceProperties													= nullptr;

	__try
	{
		//
		// Get chosen node.
		//
		DEVICE_TREE_NODE* chosenNode										= DevTreeFindNode(CHAR8_CONST_STRING("/chosen"), TRUE);
		if(!chosenNode)
			try_leave(status = EFI_OUT_OF_RESOURCES);

		//
		// Get machine signature.
		//
		UINT32 machineSignature												= 0;
		if(!EFI_ERROR(AcpiGetMachineSignature(&machineSignature)))
			DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("machine-signature"), &machineSignature, sizeof(machineSignature), TRUE);

		//
		// Get /efi/configuration-table node.
		//
		DEVICE_TREE_NODE* configTableNode									= DevTreeFindNode(CHAR8_CONST_STRING("/efi/configuration-table"), TRUE);
		if(!configTableNode)
			try_leave(status = EFI_OUT_OF_RESOURCES);

		//
		// Add tables.
		//
		EFI_CONFIGURATION_TABLE* theTable									= EfiSystemTable->ConfigurationTable;

		for(UINTN i = 0; i < EfiSystemTable->NumberOfTableEntries; i++, theTable++)
		{
			//
			// Build table name.
			//
			CHAR8 nodeName[0x40]											= {0};
			EFI_GUID* g														= &theTable->VendorGuid;
			CHAR8 CONST* guidFormat											= CHAR8_CONST_STRING("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X");
			snprintf(nodeName, ARRAYSIZE(nodeName) - 1, guidFormat, g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3], g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

			//
			// Insert node.
			//
			DEVICE_TREE_NODE* theNode										= DevTreeAddChild(configTableNode, nodeName);

			if(!theNode)
				try_leave(status = EFI_OUT_OF_RESOURCES);

			//
			// Add GUID.
			//
			DevTreeAddProperty(theNode, CHAR8_CONST_STRING("guid"), g, sizeof(EFI_GUID), FALSE);

#if DO_REPLACE_BOARD_ID
			//
			// Check for SMBIOS GUID.
			//
			if(memcmp(&theTable->VendorGuid, &EfiSmbiosTableGuid, sizeof(EfiSmbiosTableGuid)) == 0)
			{
				//
				// Get pointer to the factory EPS (Entry Point Structure).
				//
				SMBIOS_ENTRY_POINT_STRUCTURE *factoryEPS					= static_cast<SMBIOS_ENTRY_POINT_STRUCTURE*>(theTable->VendorTable);

				//
				// Get factory table length.
				//
				UINT16 tableLength											= factoryEPS->DMI.TableLength;

#if DEBUG_BOARD_ID_CSPRINTF
				CHAR8* anchorString											= static_cast<CHAR8*>(MmAllocatePool(5));
				snprintf(anchorString, 4, CHAR8_CONST_STRING("%s"), factoryEPS->AnchorString);
				MmFreePool(anchorString);

				CsPrintf(CHAR8_CONST_STRING("AnchorString................: %s\n"), anchorString);
				CsPrintf(CHAR8_CONST_STRING("Checksum....................: 0x%x\n"), factoryEPS->Checksum);
				CsPrintf(CHAR8_CONST_STRING("EntryPointLength............: 0x%x\n"), factoryEPS->EntryPointLength);
				CsPrintf(CHAR8_CONST_STRING("MajorVersion................: 0x%x\n"), factoryEPS->MajorVersion);
				CsPrintf(CHAR8_CONST_STRING("MinorVersion................: 0x%x\n"), factoryEPS->MinorVersion);
				CsPrintf(CHAR8_CONST_STRING("MaxStructureSize............: 0x%x\n"), factoryEPS->MaxStructureSize);
				CsPrintf(CHAR8_CONST_STRING("EntryPointRevision..........: 0x%x\n"), factoryEPS->EntryPointRevision);
				CsPrintf(CHAR8_CONST_STRING("FormattedArea...............: 0x%x\n"), factoryEPS->FormattedArea);

				anchorString												= static_cast<CHAR8*>(MmAllocatePool(6));
				snprintf(anchorString, 5, CHAR8_CONST_STRING("%s"), factoryEPS->DMI.AnchorString);
				MmFreePool(anchorString);
				
				CsPrintf(CHAR8_CONST_STRING("DMI.AnchorString............: %s\n"), anchorString);
				CsPrintf(CHAR8_CONST_STRING("DMI.Checksum................: 0x%x\n"), factoryEPS->DMI.Checksum);
				CsPrintf(CHAR8_CONST_STRING("DMI.TableLength.............: 0x%x\n"), factoryEPS->DMI.TableLength);
				CsPrintf(CHAR8_CONST_STRING("DMI.TableAddress............: 0x%x\n"), factoryEPS->DMI.TableAddress);
				CsPrintf(CHAR8_CONST_STRING("DMI.NumberOfSmbiosStructures: 0x%x\n"), factoryEPS->DMI.NumberOfSmbiosStructures);
				CsPrintf(CHAR8_CONST_STRING("DMI.SmbiosBcdRevision.......: 0x%x\n"), factoryEPS->DMI.SmbiosBcdRevision);
#endif
				UINT8* startOfTable											= ArchConvertAddressToPointer(factoryEPS->DMI.TableAddress, UINT8*);
				UINT8* endOfTable											= startOfTable + tableLength;
				
				//
				// Main loop, search for board info table.
				//
				while(startOfTable + sizeof(SMBIOS_TABLE_HEADER) <= endOfTable)
				{
					SMBIOS_TABLE_HEADER* tableHeader						= reinterpret_cast<SMBIOS_TABLE_HEADER*>(startOfTable);
					
					if(tableHeader->Type == 2)
					{
#if DEBUG_BOARD_ID_CSPRINTF
						CsPrintf(CHAR8_CONST_STRING("PIKE: SMBIOS kSMBTypeBaseBoard found\n"));
#endif
						if(startOfTable + sizeof(SMBIOS_TABLE_TYPE2) > endOfTable)
							break;

						SMBIOS_TABLE_TYPE2* table2							= reinterpret_cast<SMBIOS_TABLE_TYPE2*>(startOfTable);

						if (table2->ProductName)
						{
							UINT8* boardId									= BlpGetStringFromSMBIOSTable(startOfTable + table2->Hdr.Length, table2->ProductName);
#if DEBUG_BOARD_ID_CSPRINTF
							CsPrintf(CHAR8_CONST_STRING("PIKE: SMBIOS board-id found\n"));
							CsPrintf(CHAR8_CONST_STRING("PIKE: SMBIOS factory board-id: %s\n"), boardId);
#endif
							
							if (memcmp((CHAR8 *)boardId, (CHAR8 *)"Mac-F4208DC8", 12) == 0) // MacPro1,1
							{
#if DEBUG_BOARD_ID_CSPRINTF
								CsPrintf(CHAR8_CONST_STRING("PIKE: SMBIOS board-id: %s replaced with: %s\n"), (CHAR8 *)boardId, (CHAR8 *)MACPRO_41);
#endif
								memcpy((CHAR8 *)boardId, (CHAR8 *)MACPRO_41, 12);
							}
							else if (memcmp((CHAR8 *)boardId, (CHAR8 *)"Mac-F4208DA9", 12) == 0) // MacPro2,1
							{
#if DEBUG_BOARD_ID_CSPRINTF
								CsPrintf(CHAR8_CONST_STRING("PIKE: SMBIOS board-id: %s replaced with: %s\n"), (CHAR8 *)boardId, (CHAR8 *)MACPRO_41);
#endif
								memcpy((CHAR8 *)boardId, (CHAR8 *)MACPRO_41, 12);
							}
							else if (memcmp((CHAR8 *)boardId, (CHAR8 *)"Mac-F42187C8", 12) == 0)// MacBookPro2,2
							{
#if DEBUG_BOARD_ID_CSPRINTF
								CsPrintf(CHAR8_CONST_STRING("PIKE: SMBIOS board-id: %s replaced with: %s\n"), (CHAR8 *)boardId, (CHAR8 *)MACBOOKPRO_31);
#endif
								memcpy((CHAR8 *)boardId, (CHAR8 *)MACBOOKPRO_31, 12);
							}
							else if (memcmp((CHAR8 *)boardId, (CHAR8 *)"Mac-F223BEC8", 12) == 0) // Xserve1,1
							{
#if DEBUG_BOARD_ID_CSPRINTF
								CsPrintf(CHAR8_CONST_STRING("PIKE: SMBIOS board-id: %s replaced with: %s\n"), (CHAR8 *)boardId, (CHAR8 *)MACPRO_41);
#endif
								memcpy((CHAR8 *)boardId, (CHAR8 *)MACPRO_41, 12);
							}

							//
							// Do we still need this?
							//
							boardId											= Add2Ptr(boardId, 12, UINT8*);
							boardId											= 0x00;

#if DEBUG_BOARD_ID_CSPRINTF
							boardId											= BlpGetStringFromSMBIOSTable(startOfTable + table2->Hdr.Length, table2->ProductName);

							CsPrintf(CHAR8_CONST_STRING("PIKE: SMBIOS fake board-id: %s\n"), boardId);
#endif
							break;
						}
					}

					startOfTable											+= tableHeader->Length;
					
					while(startOfTable < endOfTable)
					{
						if(startOfTable[0] || startOfTable + 1 >= endOfTable || startOfTable[1])
						{
							startOfTable									+= 1;
						}
						else
						{
							startOfTable									+= 2;
							break;
						}
					}
				}
			}
#endif
			//
			// add table, (check 64bit kernel instead of EFI64?)
			//
			if(ArchNeedEFI64Mode())
			{
				UINT64 address64											= ArchConvertPointerToAddress(theTable->VendorTable);
				DevTreeAddProperty(theNode, CHAR8_CONST_STRING("table"), &address64, sizeof(address64), TRUE);
			}
			else
			{
				UINT32 address32											= static_cast<UINT32>(ArchConvertPointerToAddress(theTable->VendorTable));
				DevTreeAddProperty(theNode, CHAR8_CONST_STRING("table"), &address32, sizeof(address32), TRUE);
			}

			//
			// Add ACPI aliases.
			//
			if(!memcmp(g, &EfiAcpi20TableGuid, sizeof(EFI_GUID)))
				DevTreeAddProperty(theNode, CHAR8_CONST_STRING("alias"), "ACPI_20", 8, FALSE);
			else if(!memcmp(g, &EfiAcpiTableGuid, sizeof(EFI_GUID)))
				DevTreeAddProperty(theNode, CHAR8_CONST_STRING("alias"), "ACPI", 5, FALSE);
		}

		//
		// get /efi/runtime-services node
		//
		DEVICE_TREE_NODE* runtimeServicesNode								= DevTreeFindNode(CHAR8_CONST_STRING("/efi/runtime-services"), TRUE);

		if(!runtimeServicesNode)
			try_leave(status = EFI_OUT_OF_RESOURCES);

		//
		// add table
		//
		if(ArchNeedEFI64Mode())
		{
			UINT64 address64												= ArchConvertPointerToAddress(EfiSystemTable->RuntimeServices);
			DevTreeAddProperty(runtimeServicesNode, CHAR8_CONST_STRING("table"), &address64, sizeof(address64), TRUE);
		}
		else
		{
			UINT32 address32												= static_cast<UINT32>(ArchConvertPointerToAddress(EfiSystemTable->RuntimeServices));
			DevTreeAddProperty(runtimeServicesNode, CHAR8_CONST_STRING("table"), &address32, sizeof(address32), TRUE);
		}

		//
		// get /efi node
		//
		DEVICE_TREE_NODE* efiNode											= DevTreeFindNode(CHAR8_CONST_STRING("/efi"), FALSE);
		if(efiNode)
		{
			//
			// add firmware-vendor,firmware-revision,firmware-abi
			//
			DevTreeAddProperty(efiNode, CHAR8_CONST_STRING("firmware-vendor"), EfiSystemTable->FirmwareVendor, static_cast<UINT32>((wcslen(EfiSystemTable->FirmwareVendor) + 1) * sizeof(CHAR16)), FALSE);
			DevTreeAddProperty(efiNode, CHAR8_CONST_STRING("firmware-revision"), &EfiSystemTable->FirmwareRevision, sizeof(EfiSystemTable->FirmwareRevision), FALSE);
			DevTreeAddProperty(efiNode, CHAR8_CONST_STRING("firmware-abi"), ArchNeedEFI64Mode() ? "EFI64" : "EFI32" , 6, FALSE);

			//
			// get device-property protocol
			//
			APPLE_DEVICE_PROPERTY_PROTOCOL* devicePropertyProtocol			= nullptr;
			if(!EFI_ERROR(EfiBootServices->LocateProtocol(&AppleDevicePropertyProtocolGuid, nullptr, reinterpret_cast<VOID**>(&devicePropertyProtocol))))
			{
				//
				// loop read
				//
				EFI_STATUS status2											= EFI_BUFFER_TOO_SMALL;
				UINTN bufferLength											= 0;
				while(status2 == EFI_BUFFER_TOO_SMALL)
				{
					//
					// reallocate buffer
					//
					if(deviceProperties)
						MmFreePool(deviceProperties);

					if(!bufferLength)
						bufferLength										= 0x1000;

					deviceProperties										= MmAllocatePool(bufferLength);
					if(!deviceProperties)
						break;

					//
					// get
					//
					status2													= devicePropertyProtocol->GetDeviceProperty(devicePropertyProtocol, deviceProperties, &bufferLength);
				}

				if(!EFI_ERROR(status2) && bufferLength && deviceProperties)
					DevTreeAddProperty(efiNode, CHAR8_CONST_STRING("device-properties"), deviceProperties, static_cast<UINT32>(bufferLength), TRUE);
			}
		}

		//
		// setup compatibility in /efi/kernel-compatibility node
		//
		UINT32 compatibilityFlags											= 1;
		DEVICE_TREE_NODE* kernelCompatibilityNode							= DevTreeFindNode(CHAR8_CONST_STRING("/efi/kernel-compatibility"), TRUE);
		if(kernelCompatibilityNode)
			DevTreeAddProperty(kernelCompatibilityNode, CHAR8_CONST_STRING("x86_64"), &compatibilityFlags, sizeof(compatibilityFlags), TRUE);
	}
	__finally
	{
		if(deviceProperties)
			MmFreePool(deviceProperties);
	}

	return status;
}
