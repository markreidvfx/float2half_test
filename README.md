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
Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz
runs: 10000000
hardware            : 0.008820 secs
table no rounding   : 0.038995 secs
table with rounding : 0.048662 secs
no table            : 0.102306 secs

Raspberry Pi 4 Model B
runs: 10000000
hardware            : 0.424169 secs
table no rounding   : 0.507391 secs
table with rounding : 0.539945 secs
no table            : 0.674957 secs
```