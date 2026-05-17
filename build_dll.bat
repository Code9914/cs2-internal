@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
msbuild "D:\Projets\C++ CUserCmd Test\src\CockEngine.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Rebuild
if %errorlevel% equ 0 (
    copy /Y "D:\Projets\C++ CUserCmd Test\src\x64\Release\CockEngine.dll" "D:\Projets\C++ CUserCmd Test\cs2_internal.dll"
    echo [OK] DLL copied to cs2_internal.dll
) else (
    echo [FAIL] Build failed
)
pause
