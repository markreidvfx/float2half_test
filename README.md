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

# Results
```
CPU: x86_64 Intel(R) Xeon(R) Platinum 8171M CPU @ 2.60GHz +sse2+sse3+ssse3+sse4+sse42+avx+avx2+f16c
Linux 5.15.0-1036-azure #43-Ubuntu SMP Wed Mar 29 16:11:05 UTC 2023 x86_64 gnu gcc 11.3.

runs: 50, buffer size: 8294400, random f32 <= HALF_MAX

name                 :      min      avg     max
hardware             : 0.003672 0.003939 0.009108 secs
table no rounding    : 0.010314 0.010342 0.010392 secs
table rounding       : 0.038853 0.038911 0.038990 secs
no table             : 0.048701 0.048780 0.048867 secs
imath half           : 0.041896 0.041998 0.042169 secs
cpython              : 0.136061 0.136229 0.136457 secs
numpy                : 0.051439 0.051563 0.051788 secs
tursa                : 0.054723 0.054843 0.055627 secs
ryg                  : 0.036964 0.037232 0.039436 secs
maratyszcza          : 0.014011 0.014064 0.014153 secs
maratyszcza nan fix  : 0.014278 0.014326 0.014417 secs
maratyszcza sse2     : 0.012101 0.012151 0.012234 secs

runs: 50, buffer size: 8294400, random f32 full +inf+nan

name                 :      min      avg     max
hardware             : 0.003674 0.003900 0.004323 secs
table no rounding    : 0.010307 0.010353 0.010505 secs
table rounding       : 0.038863 0.038908 0.038999 secs
no table             : 0.071285 0.071331 0.071387 secs
imath half           : 0.068773 0.068840 0.069027 secs
cpython              : 0.186463 0.186677 0.187022 secs
numpy                : 0.076370 0.076437 0.076515 secs
tursa                : 0.077197 0.077265 0.077507 secs
ryg                  : 0.067998 0.068121 0.070234 secs
maratyszcza          : 0.012138 0.012177 0.012295 secs
maratyszcza nan fix  : 0.012417 0.012459 0.012715 secs
maratyszcza sse2     : 0.010691 0.010738 0.010799 secs

CPU: x86_64 Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz +sse2+sse3+ssse3+sse4+sse42+avx+avx2+f16c
Windows 10.0 MSVC 1929

runs: 50, buffer size: 8294400, random f32 <= HALF_MAX

name                 :      min      avg     max
hardware             : 0.002472 0.002921 0.006176 secs
table no rounding    : 0.007013 0.007580 0.009200 secs
table rounding       : 0.022742 0.023916 0.026330 secs
no table             : 0.029832 0.030624 0.032834 secs
imath half           : 0.027404 0.028323 0.030142 secs
cpython              : 0.194163 0.201640 0.209567 secs
numpy                : 0.027501 0.028982 0.030686 secs
tursa                : 0.031865 0.033007 0.034312 secs
ryg                  : 0.020279 0.020932 0.023077 secs
maratyszcza          : 0.022421 0.023268 0.025290 secs
maratyszcza nan fix  : 0.019751 0.020749 0.022228 secs
maratyszcza sse2     : 0.007998 0.008561 0.009499 secs

runs: 50, buffer size: 8294400, random f32 full +inf+nan

name                 :      min      avg     max
hardware             : 0.002539 0.002908 0.005695 secs
table no rounding    : 0.007008 0.007431 0.008483 secs
table rounding       : 0.022738 0.023931 0.025246 secs
no table             : 0.040045 0.041767 0.045215 secs
imath half           : 0.039984 0.041692 0.046772 secs
cpython              : 0.221116 0.228801 0.238792 secs
numpy                : 0.040048 0.041077 0.043608 secs
tursa                : 0.042368 0.044450 0.046873 secs
ryg                  : 0.035655 0.036700 0.039529 secs
maratyszcza          : 0.021600 0.022908 0.024116 secs
maratyszcza nan fix  : 0.018828 0.019816 0.020943 secs
maratyszcza sse2     : 0.007062 0.007556 0.008879 secs


CPU: ARM 64-bit Raspberry Pi 4 Model B Rev 1.4
Linux 5.15.61-v8+ #1579 SMP PREEMPT Fri Aug 26 11:16:44 BST 2022 aarch64 gnu gcc 10.2.1

runs: 50, buffer size: 8294400, random f32 <= HALF_MAX

name                 :      min      avg     max
hardware             : 0.012071 0.012850 0.033230 secs
table no rounding    : 0.026451 0.026565 0.028136 secs
table rounding       : 0.084227 0.084270 0.084504 secs
no table             : 0.081554 0.082911 0.083208 secs
imath half           : 0.066802 0.068076 0.068242 secs
cpython              : 0.196547 0.197695 0.198872 secs
numpy                : 0.066054 0.066849 0.067027 secs
tursa                : 0.086459 0.086558 0.086735 secs
ryg                  : 0.072824 0.072924 0.073145 secs
maratyszcza          : 0.019023 0.019215 0.021757 secs
maratyszcza nan fix  : 0.021172 0.021293 0.023333 secs

runs: 50, buffer size: 8294400, random f32 full +inf+nan

name                 :      min      avg     max
hardware             : 0.012064 0.012650 0.017590 secs
table no rounding    : 0.026515 0.026616 0.028040 secs
table rounding       : 0.084248 0.084294 0.084442 secs
no table             : 0.109667 0.109761 0.110072 secs
imath half           : 0.098180 0.098567 0.098830 secs
cpython              : 0.261748 0.261901 0.262238 secs
numpy                : 0.102358 0.102441 0.102592 secs
tursa                : 0.115300 0.115518 0.115706 secs
ryg                  : 0.095533 0.095757 0.096078 secs
maratyszcza          : 0.019031 0.019212 0.021802 secs
maratyszcza nan fix  : 0.021177 0.021297 0.023400 secs
```

# Accuracy Compared To Hardware
```
normal/denormal value matches hardware, out of 2399133696:
table no rounding    : 85.3163%
table rounding       : 100%
no table             : 100%
imath half           : 100%
cpython              : 100%
numpy                : 100%
tursa                : 99.9987%
ryg                  : 100%
maratyszcza          : 100%
maratyszcza nan fix  : 100%
maratyszcza sse2     : 100%

nan value exactly matches hardware, out of 16777214:
table no rounding    : 50%
table rounding       : 100%
no table             : 100%
imath half           : 50%
cpython              : 0.195301%
numpy                : 50%
tursa                : 0.0976503%
ryg                  : 0.195301%
maratyszcza          : 0.195301%
maratyszcza nan fix  : 100%
maratyszcza sse2     : 100%

nan is a nan value but might not match hardware, out of 16777214:
table no rounding    : 99.9024%
table rounding       : 100%
no table             : 100%
imath half           : 100%
cpython              : 100%
numpy                : 100%
tursa                : 100%
ryg                  : 100%
maratyszcza          : 100%
maratyszcza nan fix  : 100%
maratyszcza sse2     : 100%

+/-inf value matches hardware, out of 1879056386:
table no rounding    : 99.9996%
table rounding       : 100%
no table             : 100%
imath half           : 100%
cpython              : 1.06436e-07%
numpy                : 100%
tursa                : 100%
ryg                  : 100%
maratyszcza          : 100%
maratyszcza nan fix  : 100%
maratyszcza sse2     : 100%

total exact hardware match:
table no rounding    : 91.6023%
table rounding       : 100%
no table             : 100%
imath half           : 99.8047%
cpython              : 55.8599%
numpy                : 99.8047%
tursa                : 99.609%
ryg                  : 99.6101%
maratyszcza          : 99.6101%
maratyszcza nan fix  : 100%
maratyszcza sse2     : 100%
```