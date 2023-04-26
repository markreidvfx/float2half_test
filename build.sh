#!/bin/bash
set -e
machinestr=$(uname -m)
echo $machinestr

if [ "$machinestr" = 'aarch64' ]; then
# arm raspberry pi 64
gcc -O3 float2half.c -DUSE_ARM -o float2half_aarch64
gcc -O3 half2float.c -DUSE_ARM -o half2float_aarch64
./float2half_aarch64
./half2float_aarch64

# ross linux aarch64 qemu
# aarch64-linux-gnu-gcc -O3 float2half.c -DUSE_ARM -o float2half_aarch64
# aarch64-linux-gnu-gcc -O3 half2float.c -DUSE_ARM -o half2float_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./float2half_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./half2float_aarch64

elif  [ "$machinestr" = 'armv7l' ]; then
# arm raspberry pi 32
gcc -O3 -mfpu=neon-fp-armv8 -mfloat-abi=hard -mfp16-format=ieee float2half.c -DUSE_ARM  -o float2half_arm
gcc -O3 -mfpu=neon-fp-armv8 -mfloat-abi=hard -mfp16-format=ieee half2float.c -DUSE_ARM  -o half2float_arm
./float2half_arm
./half2float_arm

else
# x86
gcc -O3 -mf16c float2half.c -o float2half
gcc -O3 -mf16c half2float.c -o half2float
./float2half
./half2float

fi
