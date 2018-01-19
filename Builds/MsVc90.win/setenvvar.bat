:: This program takes a single param - the
::
::

@echo off

::Check if on-line help is required
@for /F "usebackq tokens=1,2 delims==-/ " %%i in ('%*') do @(
@if /I "%%i"=="h" (goto :HELP & goto :EOF)
@if /I "%%i"=="?" (goto :HELP & goto :EOF)
@if /I "%%i"=="HELP" (goto :HELP & goto :EOF)
)

@echo Setting environment for %2...

::=====================
:SET_FB_TARGET_PLATFORM

@if not defined FIREBIRD (
  set FIREBIRD=C:\Program Files\Firebird\Firebird_2_5
)

:: can be x86 or x64
@set FB_COMPILER_TYPE=%1

@if /I "%FB_COMPILER_TYPE%"=="AMD64" (
  @set FB_TARGET_PLATFORM=x64
) else (
  @set FB_TARGET_PLATFORM=Win32
)

::=========================
:SET_BUILDTYPE
if /I "%2" == "CLEAN" (
  set BUILDTYPE=REBUILD
) else (
  set BUILDTYPE=BUILD
)

::=========================
:SET_CONFIG
set BUILDCONFIG=release


::===============================
:: Search for and set up the compiler environment
::
:SETUP_COMPILER
@echo Guessing which compiler to use...
if DEFINED VS90COMNTOOLS (
	@"%VS90COMNTOOLS%\..\IDE\devenv" /? >nul 2>nul
	@if not errorlevel 9009 (
		call "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" %FB_COMPILER_TYPE%
	)
)
@echo.


::=================
:SET_MSVC_VER
@for /f "delims=." %%a in ('@devenv /?') do (
  @for /f "tokens=6" %%b in ("%%a") do (
	(set MSVC_VERSION=%%b) & (set VS_VER=msvc%%b) & (goto :END)
  )
)
@if not defined MSVC_VERSION goto :ERROR


goto :END


::===========
:HELP
@echo.
@echo    %0 - Usage:
@echo    This script takes the following parameters:
@echo      PLATFORM - pass 'x86' or 'AMD64' (without quotes)
@echo.
:: set errorlevel
@exit /B 1

::===========
:ERROR
@echo.
@echo    ERROR:
@echo    A working version of Visual Studio cannot be found
@echo    on your current path.
@echo.
@echo    You need MS Visual Studio 9 to build Firebird
@echo    from these batch files.
@echo.
@echo    A properly installed version of Visual Studio will set
@echo    the environment variable %%VS90COMNTOOLS%%. We use that
@echo    variable to run the appropriate batch file to set up
@echo    the build environment.
@echo.
:: set errorlevel
@exit /B 1

:END
@echo.
@echo   Building with these environment variables...
@echo.
@echo     vs_ver=%VS_VER%
if defined VS_VER_EXPRESS (
@echo     vs_ver_express=%VS_VER_EXPRESS%
)
@echo     platform=%FB_TARGET_PLATFORM%
@echo     compiler=%FB_COMPILER_TYPE%
@echo     msvc_version=%MSVC_VERSION%
@echo     firebird=%FIREBIRD%
@echo     Build type=%BUILDTYPE%
@echo     Build config=%BUILDCONFIG%
@echo.
@exit /B 0
