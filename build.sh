#!/bin/bash

cd edk2
source edksetup.sh
build
if [ $? != 0 ]; then
  exit;
fi
cd ..
mkdir UEFIdisc
cp edk2/Build/benchmarker/RELEASE_GCC5/X64/benchmarker.efi UEFIdisc
qemu-system-x86_64 -m 8G -drive if=pflash,format=raw,file=edk2/Build/OvmfX64/RELEASE_GCC5/FV/OVMF.fd -drive format=raw,file=fat:rw:UEFIdisc -net none
