set -e

# x86
gcc -mf16c -O3 float2half.c -o float2half
gcc -mf16c -O3 half2float.c -o half2float
./float2half
./half2float

# # arm raspberry pi 32
# gcc -mfpu=neon-fp-armv8 -mfloat-abi=hard -mfp16-format=ieee -O3 float2half.c -DUSE_ARM  -o float2half_arm
# gcc -mfpu=neon-fp-armv8 -mfloat-abi=hard -mfp16-format=ieee -O3 half2float.c -DUSE_ARM  -o half2float_arm
# ./float2half_arm
# ./half2float_arm

# # arm raspberry pi 64
# gcc -O3 float2half.c -DUSE_ARM  -o float2half_aarch64
# gcc-O3 half2float.c -DUSE_ARM  -o half2float_aarch64
# ./float2half_aarch64
# ./half2float_aarch64

# # cross linux aarch64 qemu
# aarch64-linux-gnu-gcc -O3 float2half.c -DUSE_ARM -o float2half_aarch64
# aarch64-linux-gnu-gcc -O3 half2float.c -DUSE_ARM -o half2float_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./float2half_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./half2float_aarch64