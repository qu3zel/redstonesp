@echo off
rem Builds RedstoneServerPanel.exe with MinGW-w64 if available, otherwise MSVC.
setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if %errorlevel%==0 (
    windres resource.rc -O coff -o resource.o || exit /b 1
    g++ -O2 -s -std=c++17 redstonesp.cpp resource.o -o RedstoneServerPanel.exe ^
        -mwindows -static -lcomdlg32 -lpsapi -lshell32 -ladvapi32 -lgdi32 ^
        -luser32 -lwinhttp -lgdiplus -lshlwapi || exit /b 1
    goto done
)

where cl >nul 2>nul
if %errorlevel%==0 (
    rc resource.rc || exit /b 1
    cl /nologo /std:c++17 /utf-8 /EHsc /O2 redstonesp.cpp resource.res /Fe:RedstoneServerPanel.exe ^
       /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib shell32.lib ^
       advapi32.lib psapi.lib winhttp.lib gdiplus.lib shlwapi.lib || exit /b 1
    goto done
)

echo No compiler found. Install MinGW-w64 or run from a Visual Studio Developer Command Prompt.
exit /b 1

:done
echo Build OK: RedstoneServerPanel.exe
