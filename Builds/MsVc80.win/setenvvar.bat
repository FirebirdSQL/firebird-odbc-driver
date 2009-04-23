@echo off
goto :MAIN %*
goto :EOF

:MSVC8_VARS
if not defined VCINSTALLDIR (
 if not defined VS80COMNTOOLS (
  goto :HELP
 ) else (
 (echo Setting up Visual Studio %1 environment...)
 "%VS80COMNTOOLS%..\..\VC\vcvarsall.bat"
 set MSVC_VERSION=8
 set VS_VER=msvc8
 )
)
goto :EOF


:SET_FB_TARGET_PLATFORM
@set FB_TARGET_PLATFORM=win32
:: If MSVC >= 8 then we can test for processor architecture
@if %MSVC_VERSION% GEQ 8 (
@if "%PROCESSOR_ARCHITECTURE%"=="x86" (set FB_TARGET_PLATFORM=win32)
@if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (set FB_TARGET_PLATFORM=x64)
)
echo.
echo    FB_TARGET_PLATFORM variable is set to %FB_TARGET_PLATFORM%
goto :EOF




:CHECK_FIREBIRD
@echo off
if not defined FIREBIRD (
 goto :ERROR
) else (
echo.
echo    FIREBIRD variable is set to %FIREBIRD%
echo.
)
goto :EOF


:HELP
@echo.
@echo MSVC8 not found.
@echo Is it installed correctly?
@echo Is it newer than version 8?
@echo.
goto :EOF

:ERROR
@echo.
@echo   The FIREBIRD environment variable is not
@echo.  defined. It should point to the root of
@echo   your Firebird install tree.
@echo.
@echo   Try something like:
@echo      set FIREBIRD="c:\program files\firebird\firebird_2_0"
@echo.
:: Attempt to execute a phony command. This will ensure
:: that ERRORLEVEL is set on exit.
cancel_script > nul 2>&1
goto :EOF

:MAIN
if "%1"=="8" (
 call :MSVC8_VARS
) else (
 call :HELP & goto :EOF
)

call :SET_FB_TARGET_PLATFORM
call :CHECK_FIREBIRD

::goto :EOF

