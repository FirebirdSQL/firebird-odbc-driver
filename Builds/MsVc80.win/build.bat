@echo off
call setenvvar.bat 8
::if errorlevel 1 (@echo errorlevel is %errorlevel%) & (goto :EOF)

@set COMPDIR="%VCINSTALLDIR%"

if "%FB_TARGET_PLATFORM%"=="win32" (
  if defined FBODBC_LOG (
    @"%VCINSTALLDIR%\Bin"\nmake  /d /p -f makefile.msvc8 %1 > nmake_win32.log 2>&1
  ) else
  @"%VCINSTALLDIR%\Bin"\nmake  -f makefile.msvc8 %1
  )
) else (
  if defined FBODBC_LOG (
    @"%VCINSTALLDIR%\Bin"\amd64\nmake  /d /p -f makefile.msvc8 %1 > nmake_amd64.log 2>&1
  ) else
    @"%VCINSTALLDIR%\Bin"\amd64\nmake  -f makefile.msvc8 %1
  )
)

goto :EOF


