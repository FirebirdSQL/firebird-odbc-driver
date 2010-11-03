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

if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
	@set PLATFORM=x64
) else (
	if "%PROCESSOR_ARCHITEW6432%" == "AMD64" (
		@set PLATFORM=x64
	) else (
		@set PLATFORM=win32
	)
)

if "%PLATFORM%" == "x64" (
	call build_platform.bat x86 %1
	call build_platform.bat AMD64 %1
) else (
	call build_platform.bat x86 %1
)

@title Build complete
goto :EOF
::=======