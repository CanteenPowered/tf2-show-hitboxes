@echo off
setlocal enabledelayedexpansion

rem Taken from https://github.com/mmozeiko/wcap/blob/main/build.cmd
where /Q cl.exe || (
    set __VSCMD_ARG_NO_LOGO=1
    for /f "tokens=*" %%i in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set VS=%%i
    if "!VS!" equ "" (
        echo ERROR: Visual Studio installation not found
        exit /b 1
    )  
    call "!VS!\VC\Auxiliary\Build\vcvarsall.bat" x86 || exit /b 1
)

if not exist %~dp0\build\build.ninja (
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B %~dp0\build
)

ninja -C %~dp0\build