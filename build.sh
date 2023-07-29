#!/bin/bash
set -e
machinestr=$(uname -m)

CC=gcc

uname -a
$CC --version

if [ "$machinestr" = 'aarch64' ] || [ "$machinestr" = 'arm64' ]; then
# arm raspberry pi 64 / arm mac
$CC -O3 -c src/hardware/hardware.c
$CC -O3 -c src/table/table.c
$CC -O3 -c src/table_round/table_round.c
$CC -O3 -c src/no_table/no_table.c
$CC -O3 -c src/cpython/cpython.c
$CC -O3 -c src/numpy/numpy.c
$CC -O3 -c src/imath/imath.c
$CC -O3 -c src/tursa/tursa.c
$CC -O3 -c src/ryg/ryg.c
$CC -O3 -c src/maratyszcza/maratyszcza.c
$CC -O3 -c src/maratyszcza_nanfix/maratyszcza_nanfix.c
$CC -O3 -c src/platform_info.c

$CC -O3 src/float2half.c platform_info.o hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o float2half_aarch64
$CC -O3 src/half2float.c platform_info.o hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o half2float_aarch64

./float2half_aarch64
./half2float_aarch64

# cross linux aarch64 qemu

# aarch64-linux-gnu-gcc -O3 -DUSE_ARM float2half.c hardware.o table.o table_round.o no_table.o imath.o -o float2half_aarch64
# aarch64-linux-gnu-gcc -O3 -DUSE_ARM half2float.c hardware.o table.o table_round.o no_table.o imath.o -o half2float_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./float2half_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./half2float_aarch64

elif  [ "$machinestr" = 'armv7l' ]; then
# arm raspberry pi 32

$CC -O3 -mfpu=neon-fp-armv8 -mfloat-abi=hard -mfp16-format=ieee -c src/hardware/hardware.c
$CC -O3 -c src/table/table.c
$CC -O3 -c src/table_round/table_round.c
$CC -O3 -c src/no_table/no_table.c
$CC -O3 -c src/cpython/cpython.c
$CC -O3 -c src/numpy/numpy.c
$CC -O3 -c src/imath/imath.c
$CC -O3 -c src/tursa/tursa.c
$CC -O3 -c src/ryg/ryg.c
$CC -O3 -c src/maratyszcza/maratyszcza.c
$CC -O3 -c src/maratyszcza_nanfix/maratyszcza_nanfix.c
$CC -03 -c src/platform_info.c

$CC -O3 src/float2half.c platform_info.o hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o float2half_arm
$CC -O3 src/half2float.c platform_info.o hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o -o half2float_arm

./float2half_arm
./half2float_arm

else
# x86
$CC -O3 -mf16c -c src/hardware/hardware.c
$CC -O3 -mtune=generic -c src/table/table.c
$CC -O3 -mtune=generic -c src/table_round/table_round.c
$CC -O3 -mtune=generic -c src/no_table/no_table.c
$CC -O3 -mtune=generic -c src/cpython/cpython.c
$CC -O3 -mtune=generic -c src/numpy/numpy.c
$CC -O3 -mtune=generic -c src/imath/imath.c
$CC -O3 -mtune=generic -c src/tursa/tursa.c
$CC -O3 -mtune=generic -c src/ryg/ryg.c
$CC -O3 -mtune=generic -c src/maratyszcza/maratyszcza.c
$CC -O3 -mtune=generic -c src/maratyszcza_nanfix/maratyszcza_nanfix.c
$CC -O3 -mtune=generic -msse2 -mno-sse3 -mno-sse4 -mno-sse4.2 -mno-avx -mno-avx2 -c src/maratyszcza_sse2/maratyszcza_sse2.c
$CC -03 -c src/platform_info.c
$CC -O3 -c src/x86_cpu_info.c
#debug sse2
# $CC -O3 -mtune=generic -msse2 -mf16c  -c maratyszcza_sse2/maratyszcza_sse2.c


$CC -O3 src/float2half.c platform_info.o x86_cpu_info.o hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o maratyszcza_sse2.o -o float2half
$CC -O3 src/half2float.c platform_info.o x86_cpu_info.o hardware.o table.o table_round.o no_table.o imath.o cpython.o numpy.o tursa.o ryg.o maratyszcza.o maratyszcza_nanfix.o maratyszcza_sse2.o -o half2float

./float2half
./half2float

fi
