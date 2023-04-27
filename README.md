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
CPU: Intel(R) Xeon(R) Platinum 8272CL CPU @ 2.60GHz
runs: 50, buffer size: 8294400, random f32 <= HALF_MAX
                     :      min      avg     max
hardware             : 0.006265 0.006447 0.010780 secs
table no rounding    : 0.008718 0.008767 0.008842 secs
table rounding       : 0.032368 0.032413 0.032547 secs
no table             : 0.038381 0.038429 0.038489 secs
imath half           : 0.046293 0.046351 0.046421 secs
cpython              : 0.156164 0.156430 0.156730 secs
numpy                : 0.036970 0.037019 0.037153 secs
tursa                : 0.045947 0.045996 0.046098 secs
ryg                  : 0.029890 0.029947 0.030076 secs
maratyszcza          : 0.011741 0.011786 0.011980 secs

runs: 50, buffer size: 8294400, random f32 full +inf+nan
                     :      min      avg     max
hardware             : 0.006271 0.006360 0.006474 secs
table no rounding    : 0.008760 0.008804 0.008873 secs
table rounding       : 0.032390 0.032431 0.032648 secs
no table             : 0.060255 0.060315 0.060476 secs
imath half           : 0.072312 0.072386 0.072600 secs
cpython              : 0.168739 0.169036 0.169394 secs
numpy                : 0.057858 0.057908 0.058092 secs
tursa                : 0.071264 0.071375 0.071453 secs
ryg                  : 0.052763 0.052829 0.052968 secs
maratyszcza          : 0.010527 0.010554 0.010586 secs


CPU: Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz
runs: 50, buffer size: 8294400, random f32 <= HALF_MAX
                     :      min      avg     max
hardware             : 0.004991 0.005469 0.008847 secs
table no rounding    : 0.006932 0.007333 0.008507 secs
table rounding       : 0.022926 0.024101 0.026052 secs
no table             : 0.026421 0.027954 0.030959 secs
imath half           : 0.023787 0.024778 0.026197 secs
cpython              : 0.102065 0.107029 0.112587 secs
numpy                : 0.024748 0.025827 0.030006 secs
tursa                : 0.027446 0.028627 0.029825 secs
ryg                  : 0.019315 0.020425 0.021491 secs
maratyszcza          : 0.008846 0.009378 0.010187 secs

runs: 50, buffer size: 8294400, random f32 full +inf+nan
                     :      min      avg     max
hardware             : 0.004963 0.005365 0.006473 secs
table no rounding    : 0.006935 0.007448 0.008512 secs
table rounding       : 0.023345 0.024306 0.025548 secs
no table             : 0.039933 0.041257 0.044344 secs
imath half           : 0.039161 0.040789 0.042772 secs
cpython              : 0.109313 0.114114 0.118531 secs
numpy                : 0.038389 0.039829 0.042775 secs
tursa                : 0.041183 0.042619 0.044397 secs
ryg                  : 0.034270 0.036141 0.038015 secs
maratyszcza          : 0.007912 0.008313 0.010726 secs


Raspberry Pi 4 Model B aarch64
runs: 50, buffer size: 8294400, random f32 <= HALF_MAX
                     :      min      avg     max
hardware             : 0.012075 0.012826 0.034404 secs
table no rounding    : 0.026495 0.026604 0.028050 secs
table rounding       : 0.084239 0.084275 0.084373 secs
no table             : 0.082274 0.082994 0.083364 secs
imath half           : 0.066960 0.068077 0.068232 secs
cpython              : 0.253655 0.282359 0.329469 secs
numpy                : 0.065991 0.066955 0.067195 secs
tursa                : 0.086471 0.086564 0.087129 secs
ryg                  : 0.072601 0.072934 0.073120 secs
maratyszcza          : 0.019049 0.019236 0.021581 secs

runs: 50, buffer size: 8294400, random f32 full +inf+nan
                     :      min      avg     max
hardware             : 0.012069 0.012539 0.017424 secs
table no rounding    : 0.026493 0.026608 0.027833 secs
table rounding       : 0.084238 0.084283 0.084575 secs
no table             : 0.109297 0.109612 0.109867 secs
imath half           : 0.098124 0.098567 0.098695 secs
cpython              : 0.257057 0.257274 0.257642 secs
numpy                : 0.102259 0.102428 0.102577 secs
tursa                : 0.115357 0.115446 0.115552 secs
ryg                  : 0.095858 0.096110 0.096275 secs
maratyszcza          : 0.019051 0.019218 0.021855 secs

accuracy compared to hardware
table no rounding    : 8.397698% err
table rounding       : 0.000000% err
no table             : 0.000000% err
imath half           : 0.195312% err
cpython              : 44.140053% err
numpy                : 0.195312% err
tursa                : 0.390983% err
ryg                  : 0.389862% err
maratyszcza          : 0.389862% err

```