@echo off
setlocal

@REM MSVC doesn't need a flag to emit the F16C instruction, just the intrinsic
cl /nologo /c hardware\hardware.c /O2 /Fohardware.obj || goto :error
cl /nologo /c table\table.c /O2 /Fotable.obj || goto :error
cl /nologo /c table_round\table_round.c /O2 /Fotable_round.obj || goto :error

cl /nologo float2half.c -O2 /link hardware.obj table.obj table_round.obj || goto :error
cl /nologo half2float.c -O2 /link hardware.obj table.obj table_round.obj || goto :error

float2half.exe || goto :error
half2float.exe || goto :error


goto :EOF
:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%W