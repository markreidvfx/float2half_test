# Float2Half

A small program to test the performance and accuracy of various float to half float methods against hardware.

The table method is based on ffmpeg version of float2half, which is base on:

http://www.fox-toolkit.org/ftp/fasthalffloatconversion.pdf

An issue with the table method is it does not do any rounding.
The x86 f16c and arm f16 hardware instructions
use IEEE 754 round to the nearest, ties to even method by default.

The table method can also lose a float32 NaN value if the signaling bits are only in
the lower 13 bits of the mantissa.

`table_float2half_round` show a branchless way of rounding and retaining all NaNs.


Additional methods have been added from this article

https://www.corsix.org/content/converting-fp32-to-fp16

# Results

## Machines without fp16 support

![macOS_10136](./images/IntelR_CoreTM_i72860QM_CPU__250GHz_macOS_10136_clang_1000.png)

![macOS_10146](./images/IntelR_XeonR_CPU___________E5620___240GHz_macOS_10146_clang_900.png)

I've only still have few CPUs that are old enough to not have not have fp16c instructions.
They are both macs and because of the OS version clang appears to be using SSE4 instructions
to vectorize the some of the implementations.

Getting more results from older machines is desirable. Also more AMD machines would be nice too.

## Machines with fp16 support

### Windows
![window_i710750_MSVC](./images/IntelR_CoreTM_i710750H_CPU__260GHz_Windows_x8664_100_MSVC_1929.png)

![window_Xeon_8272CL_MSVC_x86_64](./images/IntelR_XeonR_Platinum_8272CL_CPU__260GHz_Windows_x8664_100_MSVC_1935.png)
![window_Xeon_8272CL_MSVC_x86](./images/IntelR_XeonR_Platinum_8272CL_CPU__260GHz_Windows_x86_100_MSVC_1935.png)
![window_Xeon_E52673_gcc](./images/IntelR_XeonR_CPU_E52673_v4__230GHz_Windows_x8664_100_gnu_gcc_1310.png)
![window_Xeon_8272CL_clang](./images/IntelR_XeonR_Platinum_8272CL_CPU__260GHz_Windows_x8664_100_clang_1604.png)

![window_EPYC_7452_gcc](./images/AMD_EPYC_7452_32Core_Processor_________________Windows_x8664_100_gnu_gcc_1310.png)

### Linux

![linux_Xeon_8171M_gcc](./images/IntelR_XeonR_Platinum_8171M_CPU__260GHz_Linux_51501038azure_45Ubuntu_SMP_Mon_Apr_24_154042_UTC_2023_x8664_gnu_gcc_1130.png)

![linux_Xeon_8272CL_clang](./images/IntelR_XeonR_Platinum_8370C_CPU__280GHz_Linux_51501038azure_45Ubuntu_SMP_Mon_Apr_24_154042_UTC_2023_x8664_clang_1400.png)

![linux_i74558_gcc](./images/IntelR_CoreTM_i74558U_CPU__280GHz_Linux_515046generic_4920041Ubuntu_SMP_Thu_Aug_4_191544_UTC_2022_x8664_gnu_gcc_940.png)

### macOS

![macOS_Xeon_E51650_clang](./images/IntelR_XeonR_CPU_E51650_v2__350GHz_macOS_1265_clang_1400.png)

## ARM with fp16 support

![raspberry_pi4](./images/Raspberry_Pi_4_Model_B_Rev_14_Linux_51561v8_1579_SMP_PREEMPT_Fri_Aug_26_111644_BST_2022_aarch64_gnu_gcc_1021.png)

![macOS_m2_max](./images/Apple_M2_Max_macOS_135_clang_1403.png)

# Accuracy Compared To Hardware

![normal_denormal_value](./images/accuracy/normal_and_denormal_value_matches_hardware.png)

![nan_value_exactly_matches_hardware](./images/accuracy/nan_value_exactly_matches_hardware.png)

![nan_value_is_nan](./images/accuracy/nan_is_a_nan_value_but_might_not_match_hardware.png)

![inf_match](./images/accuracy/inf_value_matches_hardware.png)

![total_match](./images/accuracy/total_exact_hardware_match.png)