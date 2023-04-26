#!/bin/bash
set -e
machinestr=$(uname -m)
echo $machinestr

gcc -O3 -c table/table.c
gcc -O3 -c table_round/table_round.c
gcc -O3 -c no_table/no_table.c
gcc -O3 -c imath/imath.c

if [ "$machinestr" = 'aarch64' ]; then
# arm raspberry pi 64
gcc -O3 -DUSE_ARM -c hardware/hardware.c
gcc -O3 float2half.c hardware.o table.o table_round.o no_table.o imath.o -o float2half_aarch64
gcc -O3 half2float.c hardware.o table.o table_round.o no_table.o imath.o -o half2float_aarch64

./float2half_aarch64
./half2float_aarch64

# cross linux aarch64 qemu

# aarch64-linux-gnu-gcc -O3 -DUSE_ARM float2half.c hardware.o table.o table_round.o no_table.o imath.o -o float2half_aarch64
# aarch64-linux-gnu-gcc -O3 -DUSE_ARM half2float.c hardware.o table.o table_round.o no_table.o imath.o -o half2float_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./float2half_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./half2float_aarch64

elif  [ "$machinestr" = 'armv7l' ]; then
# arm raspberry pi 32
gcc -O3 -DUSE_ARM -mfpu=neon-fp-armv8 -mfloat-abi=hard -mfp16-format=ieee -c hardware/hardware.c
gcc -O3 float2half.c hardware.o table.o table_round.o no_table.o imath.o -o float2half_arm
gcc -O3 half2float.c hardware.o table.o table_round.o no_table.o imath.o -o half2float_arm

./float2half_arm
./half2float_arm

else
# x86
gcc -O3 -mf16c -c hardware/hardware.c
gcc -O3 float2half.c hardware.o table.o table_round.o no_table.o imath.o -o float2half
gcc -O3 half2float.c hardware.o table.o table_round.o no_table.o imath.o -o half2float

./float2half
./half2float

fi
