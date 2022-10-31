set -e

# x86
gcc -mf16c -O3 float2half.c -o float2half
./float2half

# # arm raspberry pi
# gcc -mfp16-format=ieee -O3 float2half.c -DUSE_ARM  -o float2half_arm
# ./float2half_arm

# # cross linux aarch64 qemu
# aarch64-linux-gnu-gcc -O3 float2half.c -DUSE_ARM -o float2half_aarch64
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./float2half_aarch64