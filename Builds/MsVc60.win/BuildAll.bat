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
:: Build the ODBC driver
::

@echo off

::Check if on-line help is required
@if /I "%1"=="-h" (goto :HELP & goto :EOF)
@if /I "%1"=="/h" (goto :HELP & goto :EOF)
@if /I "%1"=="-?" (goto :HELP & goto :EOF)
@if /I "%1"=="/?" (goto :HELP & goto :EOF)
@if /I "%1"=="HELP" (goto :HELP & goto :EOF)


@goto :MAIN
@goto :EOF

:SETUP
::==========
@set BUILDTYPE=Release
@set CLEAN=/build
for %%v in ( %1 %2 ) do (
	@if /I "%%v"=="DEBUG" (set BUILDTYPE=Debug)
	@if /I "%%v"=="CLEAN" (set CLEAN=/rebuild)
)
@goto :EOF

:BUILD
::==========
@echo Building %BUILDTYPE%
@msdev %~dp0\OdbcJdbc.dsw /MAKE "OdbcJdbcSetup - Win32 %BUILDTYPE%" "IscDbc - Win32 %BUILDTYPE%" "OdbcJdbc - Win32 %BUILDTYPE%" %CLEAN% /OUT %~dpn0.log

@echo The following errors and warnings occurred during the build:
@type %~dpn0.log | findstr error(s)
@echo.
@echo.
@echo   If no errors occurred you may now proceed to package
@echo   the driverby running
@echo.
@echo      ..\..\install\Win32\MakePackage.bat
@echo.
@echo.
@goto :EOF

:MAIN
::==========
call :SETUP %*
call :BUILD
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
@echo              This can be combined with CLEAN.
@echo.
@echo       CLEAN  Delete a previous build and rebuild
@echo              This can be combined with DEBUG.
@echo.
@echo       HELP   This help screen
@echo              This option excludes all others.
@echo.
@goto :EOF



:EOF
