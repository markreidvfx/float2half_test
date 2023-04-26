cl /nologo float2half.c -O2 || goto :error
cl /nologo half2float.c -O2 || goto :error

float2half.exe || goto :error
half2float.exe || goto :error


goto :EOF
:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%W