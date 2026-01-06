#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Register/Intel/Cpuid.h>
#include <Protocol/Smbios.h>


#define ATTRIBUTE_STR_SIZE 50
#define PAGE_SIZE 4096 // this can be assumed, since UEFI works on 4KB pages

#define CHECK_EFI_MEMORY_ATTRIBUTE(attr) if (attrs & EFI_MEMORY_##attr) { \
                                           StrCpyS(&str[i], ATTRIBUTE_STR_SIZE, L" "#attr); \
                                           i+=StrLen(L" "#attr); \
                                         }

const CHAR16 *memory_types[] = { 
    L"EfiReservedMemoryType", 
    L"EfiLoaderCode", 
    L"EfiLoaderData", 
    L"EfiBootServicesCode", 
    L"EfiBootServicesData", 
    L"EfiRuntimeServicesCode", 
    L"EfiRuntimeServicesData", 
    L"EfiConventionalMemory", 
    L"EfiUnusableMemory", 
    L"EfiACPIReclaimMemory", 
    L"EfiACPIMemoryNVS", 
    L"EfiMemoryMappedIO", 
    L"EfiMemoryMappedIOPortSpace", 
    L"EfiPalCode",
    L"EfiPersistentMemory",
    L"EfiMaxMemoryType"
};

const CHAR16 * 
memoryTypeToStr(UINT32 type) 
{ 
    if (type > sizeof(memory_types)/sizeof(CHAR16 *)) 
        return L"Unknown"; 

    return memory_types[type]; 
}

VOID 
getCPUBrandString(OUT CHAR8 *targetString)
{
  UINT32 registers[4];
  UINT32 ext;

  AsmCpuid(0x80000000, &ext, NULL,NULL,NULL);

  if (ext < 0x80000004){
      targetString[0] = '\0';
      return;
  }

  for (UINTN i = 0; i<3; i++){
    AsmCpuid(0x80000002 + i, &registers[0], &registers[1], &registers[2],&registers[3]);
    CopyMem(targetString + i * 16, registers, 16);
  }

  targetString[48] = '\0';
}

CHAR8 *
getSmbiosString (IN EFI_SMBIOS_TABLE_HEADER *record, IN UINT8 StringIndex)
{
  CHAR8 *string;

  if (StringIndex == 0) {
    return "";
  }

  string = (CHAR8 *)record + record->Length;

  for (UINTN index = 1; index < StringIndex; index++) {
    while (*string != '\0') {
      string++;
    }
    string++;
  }

  return string;
}

EFI_STATUS
printRAMBrandString(){
  EFI_SMBIOS_PROTOCOL *smbios;
  EFI_SMBIOS_HANDLE handle = SMBIOS_HANDLE_PI_RESERVED;
  EFI_SMBIOS_TABLE_HEADER *record;
  EFI_STATUS Status;
  EFI_SMBIOS_TYPE type = EFI_SMBIOS_TYPE_MEMORY_DEVICE;
  UINT8 counter = 1;

  Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID**)&smbios);

  if (EFI_ERROR(Status)){
    return Status;
  }

  while(!EFI_ERROR(smbios->GetNext(smbios,&handle,&type,&record,NULL))){
    SMBIOS_TABLE_TYPE17 *table = (SMBIOS_TABLE_TYPE17*) record;

    //slot may be empty or it cannot be read (unknown)
    if (table->Size == 0 || table->Size == 0xFFFF) {
      counter++;
      continue;
    }
    CHAR8* manufacturer = getSmbiosString(record, table->Manufacturer);
    CHAR8* part = getSmbiosString(record,table->PartNumber); 
    Print(L"DIMM %d: %a %a %u MB %u MHz\n", counter, manufacturer, part, table->Size, table->Speed);
  }

  return Status;

}

VOID
testMemoryRegion(EFI_PHYSICAL_ADDRESS base, UINT64 size, UINT64 *errCounter, UINT64 pattern) {
    UINT64 *memptr = (UINT64 *)base;
    UINT64 words = size / sizeof(UINT64);
    UINT64 privateErrCounter = 0;

    for (UINT64 i = 0; i < words; i++){
      memptr[i] = pattern;
      if (memptr[i] != pattern){
        (*errCounter)++;
        privateErrCounter++;
        if (privateErrCounter < 100){
          Print(L"Error found at address: %016LX, expected: %016LX, actual: %016LX\n", base + i * sizeof(UINT64), pattern, memptr[i]);
        }
      }

      if (i == words/4) Print(L"Pass progress: 25%%\n");
      if (i == words/2) Print(L"Pass progress: 50%%\n");
      if (i == 3*words/4) Print(L"Pass progress: 75%%\n");
    }
}


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN MemoryMapSize = 0;
  EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
  UINTN MapKey;
  UINTN DescriptorSize;
  UINT32  DescriptorVersion;
  EFI_INPUT_KEY Key;
  UINTN EventIndex;

  EFI_STATUS Status;

  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

  Status = gBS->HandleProtocol(
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **)&LoadedImage
  );

  if (EFI_ERROR (Status)){
    Print(L"HandleProtocol error: %r\n", Status);
    return Status;
  }

  //EFI_PHYSICAL_ADDRESS imageBase = (EFI_PHYSICAL_ADDRESS)LoadedImage->ImageBase;
  //UINTN imageSize = LoadedImage->ImageSize;

  //disable caching globally
  AsmDisableCache();

  //printing CPU data
  CHAR8 cpuName[49];
  getCPUBrandString(cpuName);
  Print(L"CPU: %a\n", cpuName);

  //printing ram data
  Print(L"RAM\n");
  printRAMBrandString();

  gBS->Stall(2000000);

  Status = gBS->GetMemoryMap(
        &MemoryMapSize,
        MemoryMap,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion
  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    Status = gBS->AllocatePool(
          EfiBootServicesData,
          MemoryMapSize,
          (void**)&MemoryMap
    );

    if (EFI_ERROR(Status)) {
      Print(L"AllocatePool error: %r\n", Status);
      gRT->ResetSystem(EfiResetCold, Status, 0, NULL);
      return Status;
    }

    Status = gBS->GetMemoryMap(
          &MemoryMapSize,
          MemoryMap,
          &MapKey,
          &DescriptorSize,
          &DescriptorVersion
    );

    // main loop of the program
    if (!EFI_ERROR(Status))
    {
      UINT64 errCounter = 0;
      UINT64 patterns[3] = {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xAAAAAAAAAAAAAAAAULL};
      for (UINT8 i = 0; i < 3; i++){
        EFI_MEMORY_DESCRIPTOR* desc = MemoryMap;
        UINT64 pattern = patterns[i];
        Print(L"Testing pattern: %016LX\n", pattern);
        while ((UINT8 *)desc <  (UINT8 *)MemoryMap + MemoryMapSize) {
          UINTN mappingSize =(UINTN) desc->NumberOfPages * PAGE_SIZE;

          // if memType is EfiConventionalMemory, we test it, otherwise we skip
          if (desc->Type == EfiConventionalMemory){
            Print(L"Found %s - testing range %016LX - %016LX...\n", memoryTypeToStr(desc->Type), desc->PhysicalStart, desc->PhysicalStart + mappingSize - 1);
            testMemoryRegion(desc->PhysicalStart, mappingSize, &errCounter, pattern);
          }

          //SKIP
          desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)desc + DescriptorSize);
        }
      }
      Print(L"Errors count: %llu\n", errCounter);
      gBS->FreePool(MemoryMap);
    } else {
      Print(L"GetMemoryMap with buffer error: %r\n", Status);
    }
  } else {
    Print(L"GetMemoryMap without buffer error: %r\n", Status);
  }

  Print(L"Finished testing");
  Print(L"\nPress any key to reboot...\n");

  
  gBS->WaitForEvent(
      1,
      &gST->ConIn->WaitForKey,
      &EventIndex
  );
  gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
  gRT->ResetSystem(EfiResetCold, Status, 0, NULL);
  
  return Status;
}
