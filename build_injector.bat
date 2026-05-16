@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cl.exe /std:c++17 /O2 /Fe"%~dp0injector.exe" "%~dp0injector\main.cpp"
if %ERRORLEVEL% EQU 0 (
    echo [OK] Injector built successfully
) else (
    echo [FAIL] Build failed
)
pause
