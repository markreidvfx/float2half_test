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
CPU: Intel(R) Xeon(R) CPU E5-2673 v3 @ 2.40GHz
runs: 10000000
hardware             : 0.013804 secs
table no rounding    : 0.014273 secs
table rounding       : 0.046890 secs
no table             : 0.077498 secs
imath half           : 0.073492 secs

CPU: Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz
runs: 10000000
hardware             : 0.009104 secs
table no rounding    : 0.010024 secs
table rounding       : 0.032269 secs
no table             : 0.055530 secs
imath half           : 0.053820 secs

CPU Raspberry Pi 4 Model B
runs: 10000000
hardware             : 0.040097 secs
table no rounding    : 0.040361 secs
table rounding       : 0.101880 secs
no table             : 0.131957 secs
imath half           : 0.118630 secs

accuracy compared to hardware
table no rounding    : 8.397698% err
table rounding       : 0.000000% err
no table             : 0.000000% err
imath half           : 0.195312% err

```