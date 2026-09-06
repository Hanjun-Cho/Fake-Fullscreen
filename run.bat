@echo off
setlocal
cd /d "%~dp0"

set "BUILD_DIR=build"
set "GENERATOR=MinGW Makefiles"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] cmake not found on PATH. Install CMake.
    pause
    exit /b 1
)

where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ not found on PATH. Install MinGW-w64.
    pause
    exit /b 1
)

where mingw32-make >nul 2>nul
if errorlevel 1 (
    echo [ERROR] mingw32-make not found on PATH. Add MinGW's bin directory to PATH.
    pause
    exit /b 1
)

rem If an existing cache used a different generator (e.g. Visual Studio), clear it.
if exist "%BUILD_DIR%\CMakeCache.txt" (
    findstr /C:"MinGW Makefiles" "%BUILD_DIR%\CMakeCache.txt" >nul
    if errorlevel 1 (
        echo Existing cache used a different generator; cleaning %BUILD_DIR% ...
        rmdir /S /Q "%BUILD_DIR%"
    )
)

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Configuring CMake build with generator %GENERATOR% ...
    cmake -S . -B "%BUILD_DIR%" -G "%GENERATOR%"
    if errorlevel 1 goto :fail
)

echo Building FakeFullscreen.exe...
cmake --build "%BUILD_DIR%"
if errorlevel 1 goto :fail

rem Place a copy of config.ini next to the exe if none exists yet, so the
rem first run always has an editable config in the build directory.
if not exist "%BUILD_DIR%\config.ini" (
    if exist "config.ini" copy /Y "config.ini" "%BUILD_DIR%\config.ini" >nul
)

echo.
echo Press a bound hotkey to act. Close the window to exit.
echo.

"%BUILD_DIR%\fakefullscreen.exe"
echo.
echo FakeFullscreen exited (code %ERRORLEVEL%).
pause
exit /b %ERRORLEVEL%

:fail
echo.
echo [ERROR] Build failed.
pause
exit /b 1
