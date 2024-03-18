#include "../Test.h"

static PCSTR g_pszDescription = "This sample prints firmware tables, mainly SMBIOS.";

typedef BOOL CALLBACK FN_DemoSMBIOSTableStringCallback(_In_ BYTE Index, _In_z_ PCSTR String, _In_opt_ PVOID Context);

typedef struct _DEMO_SMBIOS_STRING_FIELD
{
    ULONG Offset;
    PCWSTR Name;
} DEMO_SMBIOS_STRING_FIELD, *PDEMO_SMBIOS_STRING_FIELD;

typedef struct _DEMO_SMBIOS_TABLE_INFO
{
    BYTE Type;
    PCWSTR Name;
    _Field_size_(StringFieldCount) PDEMO_SMBIOS_STRING_FIELD StringField;
    ULONG StringFieldCount;
} DEMO_SMBIOS_TABLE_INFO, *PDEMO_SMBIOS_TABLE_INFO;

#define DEFINE_DMI_STRING(Type, Field) { UFIELD_OFFSET(SMBIOS_TABLE, Type.Field), _A2W(#Field) }

static DEMO_SMBIOS_STRING_FIELD g_Type0StringFields[] = {
    DEFINE_DMI_STRING(TYPE_0_BIOS_INFO, Vendor),
    DEFINE_DMI_STRING(TYPE_0_BIOS_INFO, Version),
    DEFINE_DMI_STRING(TYPE_0_BIOS_INFO, ReleaseDate),
};

static DEMO_SMBIOS_STRING_FIELD g_Type1StringFields[] = {
    DEFINE_DMI_STRING(TYPE_1_SYSTEM_INFO, Manufacturer),
    DEFINE_DMI_STRING(TYPE_1_SYSTEM_INFO, ProductName),
    DEFINE_DMI_STRING(TYPE_1_SYSTEM_INFO, Version),
    DEFINE_DMI_STRING(TYPE_1_SYSTEM_INFO, SerialNumber),
    DEFINE_DMI_STRING(TYPE_1_SYSTEM_INFO, SKUNumber),
    DEFINE_DMI_STRING(TYPE_1_SYSTEM_INFO, Famliy),
};

static DEMO_SMBIOS_STRING_FIELD g_Type2StringFields[] = {
    DEFINE_DMI_STRING(TYPE_2_BASEBOARD_INFO, Manufacturer),
    DEFINE_DMI_STRING(TYPE_2_BASEBOARD_INFO, Product),
    DEFINE_DMI_STRING(TYPE_2_BASEBOARD_INFO, Version),
    DEFINE_DMI_STRING(TYPE_2_BASEBOARD_INFO, SerialNumber),
    DEFINE_DMI_STRING(TYPE_2_BASEBOARD_INFO, AssetTag),
};

static DEMO_SMBIOS_STRING_FIELD g_Type3StringFields[] = {
    DEFINE_DMI_STRING(TYPE_3_SYSTEM_ENCLOSURE_OR_CHASSIS, Manufacturer),
    DEFINE_DMI_STRING(TYPE_3_SYSTEM_ENCLOSURE_OR_CHASSIS, Version),
    DEFINE_DMI_STRING(TYPE_3_SYSTEM_ENCLOSURE_OR_CHASSIS, SerialNumber),
    DEFINE_DMI_STRING(TYPE_3_SYSTEM_ENCLOSURE_OR_CHASSIS, AssetTagNumber),
};

static DEMO_SMBIOS_STRING_FIELD g_Type4StringFields[] = {
    DEFINE_DMI_STRING(TYPE_4_PROCESSOR_INFO, SocketDesignation),
    DEFINE_DMI_STRING(TYPE_4_PROCESSOR_INFO, Manufacturer),
    DEFINE_DMI_STRING(TYPE_4_PROCESSOR_INFO, Version),
    DEFINE_DMI_STRING(TYPE_4_PROCESSOR_INFO, SerialNumber),
    DEFINE_DMI_STRING(TYPE_4_PROCESSOR_INFO, AssetTag),
    DEFINE_DMI_STRING(TYPE_4_PROCESSOR_INFO, PartNumber),
};

static DEMO_SMBIOS_STRING_FIELD g_Type7StringFields[] = {
    DEFINE_DMI_STRING(TYPE_7_CACHE_INFO, SocketDesignation),
};

static DEMO_SMBIOS_STRING_FIELD g_Type8StringFields[] = {
    DEFINE_DMI_STRING(TYPE_8_PORT_CONNECTOR_INFO, InternalReferenceDesignator),
    DEFINE_DMI_STRING(TYPE_8_PORT_CONNECTOR_INFO, ExternalReferenceDesignator),
};

static DEMO_SMBIOS_STRING_FIELD g_Type9StringFields[] = {
    DEFINE_DMI_STRING(TYPE_9_SYSTEM_SLOTS, Designation),
};

static DEMO_SMBIOS_STRING_FIELD g_Type13StringFields[] = {
    DEFINE_DMI_STRING(TYPE_13_BIOS_LANGUAGE_INFO, CurrentLanguage),
};

static DEMO_SMBIOS_STRING_FIELD g_Type14StringFields[] = {
    DEFINE_DMI_STRING(TYPE_14_GROUP_ASSOCIATIONS, GroupName),
};

static DEMO_SMBIOS_STRING_FIELD g_Type17StringFields[] = {
    DEFINE_DMI_STRING(TYPE_17_MEMORY_DEVICE, DeviceLocator),
    DEFINE_DMI_STRING(TYPE_17_MEMORY_DEVICE, BankLocator),
    DEFINE_DMI_STRING(TYPE_17_MEMORY_DEVICE, Manufacturer),
    DEFINE_DMI_STRING(TYPE_17_MEMORY_DEVICE, SerialNumber),
    DEFINE_DMI_STRING(TYPE_17_MEMORY_DEVICE, AssetTag),
    DEFINE_DMI_STRING(TYPE_17_MEMORY_DEVICE, PartNumber),
    DEFINE_DMI_STRING(TYPE_17_MEMORY_DEVICE, FirmwareVersion),
};

static DEMO_SMBIOS_STRING_FIELD g_Type26StringFields[] = {
    DEFINE_DMI_STRING(TYPE_26_VOLTAGE_PROBE, Description),
};

static DEMO_SMBIOS_STRING_FIELD g_Type27StringFields[] = {
    DEFINE_DMI_STRING(TYPE_27_COOLING_DEVICE, Description),
};

static DEMO_SMBIOS_STRING_FIELD g_Type28StringFields[] = {
    DEFINE_DMI_STRING(TYPE_28_TEMPERATURE_PROBE, Description),
};

static DEMO_SMBIOS_STRING_FIELD g_Type29StringFields[] = {
    DEFINE_DMI_STRING(TYPE_29_ELECTRICAL_CURRENT_PROBE, Description),
};

static DEMO_SMBIOS_STRING_FIELD g_Type30StringFields[] = {
    DEFINE_DMI_STRING(TYPE_30_OUT_OF_BAND_REMOTE_ACCESS, ManufacturerName),
};

static DEMO_SMBIOS_STRING_FIELD g_Type34StringFields[] = {
    DEFINE_DMI_STRING(TYPE_34_MANAGEMENT_DEVICE, Description),
};

static DEMO_SMBIOS_STRING_FIELD g_Type35StringFields[] = {
    DEFINE_DMI_STRING(TYPE_35_MANAGEMENT_DEVICE_COMPONENT, Description),
};

static DEMO_SMBIOS_STRING_FIELD g_Type39StringFields[] = {
    DEFINE_DMI_STRING(TYPE_39_SYSTEM_POWER_SUPPLY, Location),
    DEFINE_DMI_STRING(TYPE_39_SYSTEM_POWER_SUPPLY, DeviceName),
    DEFINE_DMI_STRING(TYPE_39_SYSTEM_POWER_SUPPLY, Manufacturer),
    DEFINE_DMI_STRING(TYPE_39_SYSTEM_POWER_SUPPLY, SerialNumber),
    DEFINE_DMI_STRING(TYPE_39_SYSTEM_POWER_SUPPLY, AssetTagNumber),
    DEFINE_DMI_STRING(TYPE_39_SYSTEM_POWER_SUPPLY, ModelPartNumber),
    DEFINE_DMI_STRING(TYPE_39_SYSTEM_POWER_SUPPLY, RevisionLevel),
};

static DEMO_SMBIOS_STRING_FIELD g_Type41StringFields[] = {
    DEFINE_DMI_STRING(TYPE_41_ONBOARD_DEVICES_EXTENDED_INFORMATION, ReferenceDesignation),
};

static DEMO_SMBIOS_STRING_FIELD g_Type43StringFields[] = {
    DEFINE_DMI_STRING(TYPE_43_TPM_DEVICE, Description),
};

static DEMO_SMBIOS_STRING_FIELD g_Type45StringFields[] = {
    DEFINE_DMI_STRING(TYPE_45_FIRMWARE_INVENTORY_INFO, ComponentName),
    DEFINE_DMI_STRING(TYPE_45_FIRMWARE_INVENTORY_INFO, Version),
    DEFINE_DMI_STRING(TYPE_45_FIRMWARE_INVENTORY_INFO, ID),
    DEFINE_DMI_STRING(TYPE_45_FIRMWARE_INVENTORY_INFO, ReleaseDate),
    DEFINE_DMI_STRING(TYPE_45_FIRMWARE_INVENTORY_INFO, Manufacturer),
    DEFINE_DMI_STRING(TYPE_45_FIRMWARE_INVENTORY_INFO, LowestSupportedFirmwareVersion),
};

static DEMO_SMBIOS_TABLE_INFO g_Tables[] = {
    { 0, L"BIOS Information", g_Type0StringFields, ARRAYSIZE(g_Type0StringFields) },
    { 1, L"System Information", g_Type1StringFields, ARRAYSIZE(g_Type1StringFields) },
    { 2, L"Baseboard Information", g_Type2StringFields, ARRAYSIZE(g_Type2StringFields) },
    { 3, L"System Enclosure or Chassis", g_Type3StringFields, ARRAYSIZE(g_Type3StringFields) },
    { 4, L"Processor Information", g_Type4StringFields, ARRAYSIZE(g_Type4StringFields) },
    { 7, L"Cache Information", g_Type7StringFields, ARRAYSIZE(g_Type7StringFields) },
    { 8, L"Port Connector Information", g_Type8StringFields, ARRAYSIZE(g_Type8StringFields) },
    { 9, L"System Slots", g_Type9StringFields, ARRAYSIZE(g_Type9StringFields) },
    { 10, L"On Board Devices Information (Obsolete)", NULL, 0 },
    { 11, L"OEM Strings", NULL, 0 },
    { 12, L"System Configuration Options", NULL, 0 },
    { 13, L"BIOS Language Information", g_Type13StringFields, ARRAYSIZE(g_Type13StringFields) },
    { 14, L"Group Associations", g_Type14StringFields, ARRAYSIZE(g_Type14StringFields) },
    { 16, L"Physical Memory Array", NULL, 0 },
    { 17, L"Memory Device", g_Type17StringFields, ARRAYSIZE(g_Type17StringFields) },
    { 19, L"Memory Array Mapped Address", NULL, 0 },
    { 20, L"Memory Device Mapped Address", NULL, 0 },
    { 23, L"System Reset", NULL, 0 },
    { 24, L"Hardware Security", NULL, 0 },
    { 25, L"System Power Controls", NULL, 0 },
    { 26, L"Voltage Probe", g_Type26StringFields, ARRAYSIZE(g_Type26StringFields) },
    { 27, L"Cooling Device", g_Type27StringFields, ARRAYSIZE(g_Type27StringFields) },
    { 28, L"Temperature Probe", g_Type28StringFields, ARRAYSIZE(g_Type28StringFields) },
    { 29, L"Electrical Current Probe", g_Type29StringFields, ARRAYSIZE(g_Type29StringFields) },
    { 30, L"Out-of-Band Remote Access", g_Type30StringFields, ARRAYSIZE(g_Type30StringFields) },
    { 32, L"System Boot Information", NULL, 0 },
    { 34, L"Management Device", g_Type34StringFields, ARRAYSIZE(g_Type34StringFields) },
    { 35, L"Management Device Component", g_Type35StringFields, ARRAYSIZE(g_Type35StringFields) },
    { 39, L"System Power Supply", g_Type39StringFields, ARRAYSIZE(g_Type39StringFields) },
    { 41, L"Onboard Devices Extended Information", g_Type41StringFields, ARRAYSIZE(g_Type41StringFields) },
    { 43, L"TPM Device", g_Type43StringFields, ARRAYSIZE(g_Type43StringFields) },
    { 44, L"Processor Additional Information", NULL, 0 },
    { 45, L"Firmware Inventory Information", g_Type45StringFields, ARRAYSIZE(g_Type45StringFields) },
    { 127, L"End-of-Table", NULL, 0 },
};

static VOID PrintSMBIOSTableInfo(_In_ PSMBIOS_TABLE Table)
{
    if (Table->Header.Type == 0)
    {
        PrintF("\tRelease Version: %u.%u\n",
                 Table->TYPE_0_BIOS_INFO.MajorRelease,
                 Table->TYPE_0_BIOS_INFO.MinorRelease);
        PrintF("\tEC Firmware Release Version: %u.%u\n",
                 Table->TYPE_0_BIOS_INFO.ECFirmwareMajorRelease,
                 Table->TYPE_0_BIOS_INFO.ECFirmwareMinorRelease);
    } else if (Table->Header.Type == 4)
    {
        PrintF("\tID: 0x%016llX\n", Table->TYPE_4_PROCESSOR_INFO.ID);
        if (Table->TYPE_4_PROCESSOR_INFO.MaxSpeed != 0)
        {
            PrintF("\tMaxSpeed: %u MHz\n", Table->TYPE_4_PROCESSOR_INFO.MaxSpeed);
        }
        if (Table->TYPE_4_PROCESSOR_INFO.CurrentSpeed != 0)
        {
            PrintF("\tCurrentSpeed: %u MHz\n", Table->TYPE_4_PROCESSOR_INFO.CurrentSpeed);
        }
        if (Table->TYPE_4_PROCESSOR_INFO.CoreCount != 0)
        {
            PrintF("\tCoreCount: %u\n", Table->TYPE_4_PROCESSOR_INFO.CoreCount);
        }
        if (Table->TYPE_4_PROCESSOR_INFO.ThreadCount != 0)
        {
            PrintF("\tThreadCount: %u\n", Table->TYPE_4_PROCESSOR_INFO.ThreadCount);
        }
    } else if (Table->Header.Type == 16)
    {
        if (Table->TYPE_16_PHYSICAL_MEMORY_ARRAY.MaximumCapacity != 0x80000000)
        {
            PrintF("\tMaximumCapacity: %u KB\n", Table->TYPE_16_PHYSICAL_MEMORY_ARRAY.MaximumCapacity);
        }
    } else if (Table->Header.Type == 17)
    {
        PrintF("\tSpeed: %u MT/s\n", Table->TYPE_17_MEMORY_DEVICE.Speed);
        if (Table->TYPE_17_MEMORY_DEVICE.ConfiguredMemorySpeed != 0x0000 &&
            Table->TYPE_17_MEMORY_DEVICE.ConfiguredMemorySpeed != 0xFFFF)
        {
            PrintF("\tConfiguredMemorySpeed: %u MT/s\n", Table->TYPE_17_MEMORY_DEVICE.ConfiguredMemorySpeed);
        }
        if (Table->TYPE_17_MEMORY_DEVICE.MinimumVoltage != 0)
        {
            PrintF("\tMinimumVoltage: %u mV\n", Table->TYPE_17_MEMORY_DEVICE.MinimumVoltage);
        }
        if (Table->TYPE_17_MEMORY_DEVICE.MaximumVoltage != 0)
        {
            PrintF("\tMaximumVoltage: %u mV\n", Table->TYPE_17_MEMORY_DEVICE.MaximumVoltage);
        }
        if (Table->TYPE_17_MEMORY_DEVICE.ConfiguredVoltage != 0)
        {
            PrintF("\tConfiguredVoltage: %u mV\n", Table->TYPE_17_MEMORY_DEVICE.ConfiguredVoltage);
        }
    } else if (Table->Header.Type == 39)
    {
        if (Table->TYPE_39_SYSTEM_POWER_SUPPLY.MaxPowerCapacity != 0x8000)
        {
            PrintF("\tMaxPowerCapacity: %u mW\n", Table->TYPE_39_SYSTEM_POWER_SUPPLY.MaxPowerCapacity);
        }
    } else if (Table->Header.Type == 43)
    {
        PrintF("\tVendorID: %c%c%c%c\n",
                 Table->TYPE_43_TPM_DEVICE.VendorID[0],
                 Table->TYPE_43_TPM_DEVICE.VendorID[1],
                 Table->TYPE_43_TPM_DEVICE.VendorID[2],
                 Table->TYPE_43_TPM_DEVICE.VendorID[3]);
        PrintF("\tSpecVersion: %u.%u\n",
                 Table->TYPE_43_TPM_DEVICE.MajorSpecVersion,
                 Table->TYPE_43_TPM_DEVICE.MinorSpecVersion);
        PrintF("\tOEMDefined: 0x%08lX\n", Table->TYPE_43_TPM_DEVICE.OEMDefined);
    } else if (Table->Header.Type == 45)
    {
        PrintF("\tImageSize: 0x%016llX\n", Table->TYPE_45_FIRMWARE_INVENTORY_INFO.ImageSize);
    }
}

static BOOL CALLBACK SMBIOSTableStringCallback(_In_ BYTE Index, _In_z_ PCSTR String, _In_opt_ PVOID Context)
{
    PSMBIOS_TABLE SMBIOSTable;
    ULONG i, j;

    if (Context == NULL)
    {
        return FALSE;
    }

    SMBIOSTable = Context;
    for (i = 0; i < ARRAYSIZE(g_Tables); i++)
    {
        if (g_Tables[i].Type == SMBIOSTable->Header.Type)
        {
            break;
        }
    }
    if (i >= ARRAYSIZE(g_Tables))
    {
        return TRUE;
    }
    if (g_Tables[i].StringFieldCount > 0)
    {
        for (j = 0; j < g_Tables[i].StringFieldCount; j++)
        {
            if (g_Tables[i].StringField[j].Offset < SMBIOSTable->Header.Length &&
                *((PBYTE)SMBIOSTable + g_Tables[i].StringField[j].Offset) == Index)
            {
                PrintF("\t%ls: %hs\n", g_Tables[i].StringField[j].Name, String);
            }
        }
    } else if (SMBIOSTable->Header.Type == 10 ||
               SMBIOSTable->Header.Type == 11 ||
               SMBIOSTable->Header.Type == 12)
    {
        PrintF("\t%hs\n", String);
    }

    return TRUE;
}

static
_Ret_maybenull_
PSMBIOS_TABLE EnumSMBIOSString(
    _In_ PSMBIOS_RAWDATA SMBIOSData,
    _In_ PSMBIOS_TABLE Table,
    _In_opt_ FN_DemoSMBIOSTableStringCallback* Callback,
    _In_opt_ PVOID Context)
{
    PCSTR psz;
    BYTE i;
    BOOL Continue;
    PVOID pEnd;
    SIZE_T sMaxCount, sLen;

    pEnd = Add2Ptr(SMBIOSData->SMBIOSTableData, SMBIOSData->Length);
    psz = (PCSTR)Add2Ptr(Table, Table->Header.Length);
    if (*(PWORD)psz == 0)
    {
        return (PSMBIOS_TABLE)Add2Ptr(psz, sizeof(WORD));
    }

    i = 1;
    Continue = TRUE;
    while (psz + sizeof(WORD) < (PCSTR)pEnd)
    {
        sMaxCount = PtrOffset(psz, pEnd);
        sLen = strnlen(psz, sMaxCount);
        if (sLen == sMaxCount)
        {
            break;
        }

        if (Continue && Callback != NULL)
        {
            Continue = Callback(i, psz, Context);
        }

        psz += sLen + 1;
        if (psz[0] == 0)
        {
            return (PSMBIOS_TABLE)(psz + 1);
        }
        i++;
    };

    return NULL;
}

BOOL Sample_PrintFirmwareTable()
{
    NTSTATUS Status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION FirmwareInfo;
    PSMBIOS_RAWDATA SMBIOSRawData;
    PSMBIOS_TABLE SMBIOSTable;
    ULONG i;

    PrintTitle(__FUNCTION__, g_pszDescription);

    /* Print SMBIOS Info */

    Status = HW_GetFirmwareTable('RSMB', 0, SystemFirmwareTable_Get, &FirmwareInfo, NULL);
    if (!NT_SUCCESS(Status))
    {
        PrintF("HW_GetFirmwareTable failed with 0x%08lX\n", Status);
        return FALSE;
    }
    SMBIOSRawData = (PSMBIOS_RAWDATA)FirmwareInfo->TableBuffer;
    if (FirmwareInfo->TableBufferLength < sizeof(*SMBIOSRawData) ||
        FirmwareInfo->TableBufferLength != sizeof(*SMBIOSRawData) + SMBIOSRawData->Length)
    {
        PrintF("SMBIOS Data size incorrect\n");
        return FALSE;
    }

    PrintF("SMBIOS Version: %u.%u\n", SMBIOSRawData->SMBIOSMajorVersion, SMBIOSRawData->SMBIOSMinorVersion);
    PrintF("DMI Revision: %u\n", SMBIOSRawData->DmiRevision);
    PrintF("Data Size: %u\n\n", SMBIOSRawData->Length);

    SMBIOSTable = (PSMBIOS_TABLE)SMBIOSRawData->SMBIOSTableData;
    do
    {
        for (i = 0; i < ARRAYSIZE(g_Tables); i++)
        {
            if (g_Tables[i].Type == SMBIOSTable->Header.Type)
            {
                PrintF("[Type %u - %ls]\n", SMBIOSTable->Header.Type, g_Tables[i].Name);
                break;
            }
        }
        if (i >= ARRAYSIZE(g_Tables))
        {
            PrintF("[Type %u]\n", SMBIOSTable->Header.Type);
        }
        PrintSMBIOSTableInfo(SMBIOSTable);
        SMBIOSTable = EnumSMBIOSString(SMBIOSRawData, SMBIOSTable, SMBIOSTableStringCallback, SMBIOSTable);
    } while (SMBIOSTable != NULL && (PBYTE)SMBIOSTable < SMBIOSRawData->SMBIOSTableData + SMBIOSRawData->Length);

    RtlFreeHeap(NtGetProcessHeap(), 0, FirmwareInfo);

    /* Get ACPI Info */

    Status = HW_GetFirmwareTable('ACPI', 'PCAF', SystemFirmwareTable_Get, &FirmwareInfo, NULL);
    if (!NT_SUCCESS(Status))
    {
        PrintF("HW_GetFirmwareTable failed with 0x%08lX\n", Status);
        return FALSE;
    }

    RtlFreeHeap(NtGetProcessHeap(), 0, FirmwareInfo);

    return TRUE;
}
