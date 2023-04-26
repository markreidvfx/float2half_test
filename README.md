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
runs: 24, buffer size: 8294400
                     :      min      avg     max
hardware             : 0.008696 0.008944 0.013575 secs
table no rounding    : 0.009876 0.009975 0.010286 secs
table rounding       : 0.032589 0.032768 0.032862 secs
no table             : 0.071695 0.071938 0.072825 secs
imath half           : 0.069788 0.069915 0.070130 secs

CPU: Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz
runs: 24, buffer size: 8294400
                     :      min      avg     max
hardware             : 0.004990 0.005486 0.006658 secs
table no rounding    : 0.007825 0.008273 0.009062 secs
table rounding       : 0.023162 0.024618 0.028471 secs
no table             : 0.041659 0.043078 0.045137 secs
imath half           : 0.039517 0.041032 0.042969 secs

CPU Raspberry Pi 4 Model B
runs: 24, buffer size: 8294400
                     :      min      avg     max
hardware             : 0.012348 0.014196 0.047234 secs
table no rounding    : 0.032063 0.032174 0.032361 secs
table rounding       : 0.084253 0.084286 0.084329 secs
no table             : 0.109903 0.110605 0.114528 secs
imath half           : 0.098605 0.098768 0.099076 secs

accuracy compared to hardware
table no rounding    : 8.397698% err
table rounding       : 0.000000% err
no table             : 0.000000% err
imath half           : 0.195312% err

```