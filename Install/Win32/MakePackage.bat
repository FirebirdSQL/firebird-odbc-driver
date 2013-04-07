::  Initial Developer's Public License.
::  The contents of this file are subject to the  Initial Developer's Public
::  License Version 1.0 (the "License"). You may not use this file except
::  in compliance with the License. You may obtain a copy of the License at
::    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
::  Software distributed under the License is distributed on an "AS IS" basis,
::  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
::  for the specific language governing rights and limitations under the
::  License.
::
::  The Original Code is copyright 2004 Paul Reeves.
::
::  The Initial Developer of the Original Code is Paul Reeves
::
::  All Rights Reserved.
::
::=============================================================================
::
:: Take a build and package it.
::
@echo off


::Check if on-line help is required
@for /F "usebackq tokens=1,2 delims==-/ " %%i in ('%*') do @(
@if /I "%%i"=="h" (goto :HELP & goto :EOF)
@if /I "%%i"=="?" (goto :HELP & goto :EOF)
@if /I "%%i"=="HELP" (goto :HELP & goto :EOF)
)


@goto :MAIN
@goto :EOF



:SET_ENVIRONMENT
::Assume we are preparing a production build
if not defined BUILDCONFIG (set BUILDCONFIG=release)

:: See what we have on the command line
for %%v in ( %* )  do (
  ( if /I "%%v"=="DEBUG" (set BUILDCONFIG=debug) )
)

@cd ..\..
@for /f "delims=" %%a in ('@cd') do (set ROOT_PATH=%%a)
@cd %~dp0

if not defined FB_TARGET_PLATFORM (
  @if "%PROCESSOR_ARCHITECTURE%"=="x86" (set FB_TARGET_PLATFORM=Win32)
  @if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (set FB_TARGET_PLATFORM=x64)
)

@goto :EOF


:SED_MAGIC
:: Do some sed magic to make sure that the final product
:: includes the version string in the filename.
:: If the Firebird Unix tools for Win32 aren't on
:: the path this will fail! Use of the cygwin tools has not
:: been tested and may produce unexpected results.
::========================================================
sed /"#define BUILDNUM_VERSION"/!d %ROOT_PATH%\WriteBuildNo.h > %temp%.\b$1.bat
sed -n -e s/\"//g -e s/"#define BUILDNUM_VERSION"//w%temp%.\b$2.bat %temp%.\b$1.bat
for /f "tokens=*" %%a in ('type %temp%.\b$2.bat') do set PRODUCT_VER_STRING=2.0.2.%%a
@echo s/1.2.0/%PRODUCT_VER_STRING%/ > %temp%.\b$3.bat
@echo s/define MSVC_VERSION 6/define MSVC_VERSION %MSVC_VERSION%/ >> %temp%.\b$3.bat
@echo s/define BUILDCONFIG release/define BUILDCONFIG %BUILDCONFIG%/ >> %temp%.\b$3.bat
@echo s/PRODUCT_VER_STRING/%PRODUCT_VER_STRING%/ >> %temp%.\b$3.bat
@set PRODUCT_VERSION=%PRODUCT_VER_STRING%
sed -f  %temp%.\b$3.bat %~dp0\OdbcJdbcSetup.iss > %~dp0\OdbcJdbcSetup_%PRODUCT_VER_STRING%.iss
del %temp%.\b$?.bat
@goto :EOF


:BUILD_HELP
::=========
if exist "%ProgramFiles%\HTML Help Workshop\hhc.exe" (
set HTMLHELP="%ProgramFiles%\HTML Help Workshop\hhc.exe"
) else (
  if exist "%ProgramFiles(x86)%\HTML Help Workshop\hhc.exe" (
	set HTMLHELP="%ProgramFiles(x86)%\HTML Help Workshop\hhc.exe"
  ) else (
  echo HTML Help Workshop is not available
  goto :EOF
  )
)
%HTMLHELP% %ROOT_PATH%\Install\HtmlHelp\OdbcJdbc.hhp
::echo ERRORLEVEL is %ERRORLEVEL%

goto :EOF


:ISX
::========
if NOT DEFINED INNO5_SETUP_PATH (set INNO5_SETUP_PATH="%PROGRAMFILES%\Inno Setup")
@Echo Now let's compile the InnoSetup scripts
@Echo.
%INNO5_SETUP_PATH%\iscc "%ROOT_PATH%\Install\Win32\OdbcJdbcSetup_%PRODUCT_VER_STRING%.iss"
goto :EOF


:HELP
::==========
@echo.
@echo.
@echo   Parameters can be passed in any order.
@echo   Parameters are NOT case-sensitive.
@echo   Currently the recognised params are:
@echo.
@echo       DEBUG  Create a DEBUG build.
@echo.
@echo       HELP   This help screen
@echo              This option excludes all others.
@echo.
goto :EOF


:MAKE_PACKAGE
::============
@Echo.
@Echo Setting environment...
@(@call :SET_ENVIRONMENT %* )|| (@echo Error calling SET_ENVIRONMENT & @goto :EOF)
@Echo.
@Echo Setting version number...
@(@call :SED_MAGIC ) || (@echo Error calling SED_MAGIC & @goto :EOF)
@Echo.
@Echo Building help file...
::Note errorlevel seems to be set to 1, even if compiler completes successfully
::So testing for an error seems pointless.
@(@call :BUILD_HELP ) & (@if ERRORLEVEL 2 (@echo Error %ERRORLEVEL% calling BUILD_HELP & @goto :EOF))
@Echo.
@Echo Building Installable Binary...
@(@call :ISX ) || (@echo Error calling Inno Setup Extensions & @goto :EOF)
@Echo.

goto :EOF


:MAIN
::====
call :MAKE_PACKAGE %*
goto :EOF


:EOF
