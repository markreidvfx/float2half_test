#!/bin/bash
set -e
machinestr=$(uname -m)

CC=gcc

uname -a
$CC --version

if [ "$machinestr" = 'aarch64' ]; then
# arm raspberry pi 64
$CC -O3 -DUSE_ARM -c hardware/hardware.c
$CC -O3 -c table/table.c
$CC -O3 -c table_round/table_round.c
$CC -O3 -c no_table/no_table.c
$CC -O3 -c cpython/cpython.c
$CC -O3 -c numpy/numpy.c
$CC -O3 -c imath/imath.c
$CC -O3 -c tursa/tursa.c
$CC -O3 -c ryg/ryg.c
$CC -O3 -c maratyszcza/maratyszcza.c
$CC -O3 -c maratyszcza_nanfix/maratyszcza_nanfix.c

$CC -O3 float2half.c hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o float2half_aarch64
$CC -O3 half2float.c hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o half2float_aarch64

./float2half_aarch64
./half2float_aarch64

# cross linux aarch64 qemu

# aarch64-linux-gnu-gcc -O3 -DUSE_ARM float2half.c hardware.o table.o table_round.o no_table.o imath.o -o float2half_aarch64
# aarch64-linux-gnu-gcc -O3 -DUSE_ARM half2float.c hardware.o table.o table_round.o no_table.o imath.o -o half2float_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./float2half_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./half2float_aarch64

elif  [ "$machinestr" = 'armv7l' ]; then
# arm raspberry pi 32

$CC -O3 -DUSE_ARM -mfpu=neon-fp-armv8 -mfloat-abi=hard -mfp16-format=ieee -c hardware/hardware.c
$CC -O3 -c table/table.c
$CC -O3 -c table_round/table_round.c
$CC -O3 -c no_table/no_table.c
$CC -O3 -c cpython/cpython.c
$CC -O3 -c numpy/numpy.c
$CC -O3 -c imath/imath.c
$CC -O3 -c tursa/tursa.c
$CC -O3 -c ryg/ryg.c
$CC -O3 -c maratyszcza/maratyszcza.c
$CC -O3 -c maratyszcza_nanfix/maratyszcza_nanfix.c

$CC -O3 float2half.c hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o float2half_arm
$CC -O3 half2float.c hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o half2float_arm

./float2half_arm
./half2float_arm

else
# x86
$CC -O3 -mf16c -c hardware/hardware.c
$CC -O3 -mtune=generic -c table/table.c
$CC -O3 -mtune=generic -c table_round/table_round.c
$CC -O3 -mtune=generic -c no_table/no_table.c
$CC -O3 -mtune=generic -c cpython/cpython.c
$CC -O3 -mtune=generic -c numpy/numpy.c
$CC -O3 -mtune=generic -c imath/imath.c
$CC -O3 -mtune=generic -c tursa/tursa.c
$CC -O3 -mtune=generic -c ryg/ryg.c
$CC -O3 -mtune=generic -c maratyszcza/maratyszcza.c
$CC -O3 -mtune=generic -c maratyszcza_nanfix/maratyszcza_nanfix.c


$CC -O3 float2half.c hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o float2half
$CC -O3 half2float.c hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o half2float

./float2half
./half2float

fi
