@echo off
::  build.bat  —  Builds ColorOverlay.aex
::
::  Prerequisite: Run from a VS x64 Developer Command Prompt, or let
::  GitHub Actions call this after ilammy/msvc-dev-cmd sets up the env.
::
::  Environment variables expected:
::    SDK_DIR   path to AE SDK root (contains Headers/, Util/, Headers/SP/)
::              Defaults to "SDK" (relative to repo root)
::
::  Output:
::    ColorOverlay.aex   (in repo root)

setlocal EnableExtensions

if "%SDK_DIR%"=="" set SDK_DIR=SDK

:: ── 1. Generate PiPL binary ──────────────────────────────────────────────────
echo [build] Generating PiPL binary...
python scripts\gen_pipl.py
if %ERRORLEVEL% neq 0 ( echo ERROR: gen_pipl.py failed & exit /b 1 )

:: ── 2. Compile .rc → .res ────────────────────────────────────────────────────
echo [build] Compiling resources...
mkdir obj 2>nul
pushd src
rc.exe /nologo /fo "..\obj\ColorOverlay.res" ColorOverlay.rc
popd
if %ERRORLEVEL% neq 0 ( echo ERROR: rc.exe failed & exit /b 1 )

:: ── 3. Compile C++ ───────────────────────────────────────────────────────────
echo [build] Compiling C++...
cl.exe /nologo /c /O2 /W3 /WX- /MT /EHsc /std:c++17 ^
    /DWIN32 /D_WINDOWS /DmsWindows /DAE_OS_WIN /DAE_PROC_INTELx64 /DNDEBUG ^
    /I"%SDK_DIR%\Headers" ^
    /I"%SDK_DIR%\Util" ^
    /I"%SDK_DIR%\Headers\SP" ^
    /Fo"obj\ColorOverlay.obj" ^
    src\ColorOverlay.cpp
if %ERRORLEVEL% neq 0 ( echo ERROR: cl.exe failed & exit /b 1 )

:: ── 4. Link ──────────────────────────────────────────────────────────────────
echo [build] Linking...
link.exe /nologo /DLL /SUBSYSTEM:WINDOWS ^
    /OUT:ColorOverlay.aex ^
    /DEF:src\ColorOverlay.def ^
    obj\ColorOverlay.obj ^
    obj\ColorOverlay.res ^
    kernel32.lib user32.lib
if %ERRORLEVEL% neq 0 ( echo ERROR: link.exe failed & exit /b 1 )

echo.
echo [build] SUCCESS: ColorOverlay.aex
echo [build] Install: copy ColorOverlay.aex to your AE Plug-ins\Effects folder
exit /b 0
