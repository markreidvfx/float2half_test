@echo off
setlocal

@REM MSVC doesn't need a flag to emit the F16C instruction, just the intrinsic
cl /nologo /c hardware\hardware.c /O2 /Fohardware.obj || goto :error
cl /nologo /c table\table.c /O2 /Fotable.obj || goto :error
cl /nologo /c table_round\table_round.c /O2 /Fotable_round.obj || goto :error
cl /nologo /c no_table\no_table.c /O2 /Fono_table.obj || goto :error
cl /nologo /c imath\imath.c /O2 /Foimath.obj || goto :error
cl /nologo /c cpython\cpython.c /O2 /Focpython.obj || goto :error
cl /nologo /c numpy\numpy.c /O2 /Fonumpy.obj || goto :error
cl /nologo /c tursa\tursa.c /O2 /Fotursa.obj || goto :error
cl /nologo /c ryg\ryg.c /O2 /Foryg.obj || goto :error
cl /nologo /c maratyszcza\maratyszcza.c /O2 /Fomaratyszcza.obj || goto :error
cl /nologo /c maratyszcza_nanfix\maratyszcza_nanfix.c /O2 /Fomaratyszcza_nanfix.obj || goto :error
cl /nologo /c maratyszcza_sse2\maratyszcza_sse2.c /O2 /Fomaratyszcza_sse2.obj || goto :error


cl /nologo float2half.c -O2 /link hardware.obj table.obj table_round.obj no_table.obj imath.obj cpython.obj numpy.obj tursa.obj ryg.obj maratyszcza.obj maratyszcza_nanfix.obj maratyszcza_sse2.obj || goto :error
cl /nologo half2float.c -O2 /link hardware.obj table.obj table_round.obj no_table.obj imath.obj cpython.obj numpy.obj tursa.obj ryg.obj maratyszcza.obj maratyszcza_nanfix.obj maratyszcza_sse2.obj || goto :error

float2half.exe || goto :error
half2float.exe || goto :error


goto :EOF
:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%W