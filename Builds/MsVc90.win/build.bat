::
:: This file is intended to do automated release builds.
:: Automated debug builds will require you to do some hacking
::
:: The only meaningful option this file understands is CLEAN
:: If you pass that it will do a rebuild of the driver.
::
:: After checking whether the binaries need to be (re)built
:: it will package up the driver.
::
:: If the host environment is 64-bit it will build and package
:: the 32-bit driver too.
::
:: WARNING: Only minimal error checking is done. The build/packaging
:: process might not abort on error - be sure to check the logs and
:: screen output afterwards.
::

@echo off
goto :MAIN & goto :EOF

:MAIN
::===========

for %%p in ( x86 AMD64 ) do (
  if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
    if "%%p" == "AMD64" (
      call %windir%\system32\cmd.exe /c build_platform.bat AMD64 %1 
    ) else (
      call %windir%\SYSWOW64\cmd.exe /c  build_platform.bat x86 %1
    )
  )
  if "%PROCESSOR_ARCHITECTURE%" == "x86" (
    if "%%p" == "x86" (
      call %windir%\system32\cmd.exe /c build_platform.bat x86 %1
    )
  )
)
@title Build complete
goto :EOF

if /I "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
  echo now building %2 %1 in 32-bit env.
  call %windir%\SYSWOW64\cmd.exe /c %0 %1
)


goto :EOF
::=======

