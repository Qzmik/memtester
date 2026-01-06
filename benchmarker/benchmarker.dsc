[Defines]
DSC_SPECIFICATION	= 0x0001001C
PLATFORM_GUID 		= 117593fd-ff3c-498e-8ded-10bea268a0b6
PLATFORM_VERSION 	= 0.01
PLATFORM_NAME		= benchmarker
SKUID_IDENTIFIER	= DEFAULT
SUPPORTED_ARCHITECTURES	= X64
BUILD_TARGETS 		= RELEASE
SKUID_IDENTIFIER = DEFAULT
FMP_SUPPORT = FALSE

[Packages]
    MdePkg/MdePkg.dec
    MdeModulePkg/MdeModulePkg.dec
    

[LibraryClasses]
UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
StackCheckLib|MdePkg/Library/StackCheckLib/StackCheckLib.inf
StackCheckFailureHookLib|MdePkg/Library/StackCheckFailureHookLibNull/StackCheckFailureHookLibNull.inf
MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

[Components]
./benchmarker/benchmarker.inf
