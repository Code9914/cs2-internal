@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /nologo /O2 /EHsc "D:\AI\C++ Internal\injector\main.cpp" /Fe:"D:\AI\C++ Internal\injector.exe" /link advapi32.lib
pause
