@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
msbuild "D:\AI\C++ Internal\src\CockEngine.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Rebuild
if %errorlevel% equ 0 (
    copy /Y "D:\AI\C++ Internal\src\x64\Release\CockEngine.dll" "D:\AI\C++ Internal\cs2_internal.dll"
    echo [OK] DLL copied to cs2_internal.dll
) else (
    echo [FAIL] Build failed
)
pause
