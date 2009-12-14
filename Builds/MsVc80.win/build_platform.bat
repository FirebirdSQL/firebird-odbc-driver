
:BUILD
::========
@call setenvvar.bat %1
@title %BUILDTYPE%  %PROCESSOR_ARCHITECTURE%
@echo   %BUILDTYPE% %PROCESSOR_ARCHITECTURE%
@set > build_%PROCESSOR_ARCHITECTURE%.log
if defined USE_NMAKE (
@echo using NMAKE
@nmake /i /f %~dp0\makefile.%vs_ver% %1 all >> build_%PROCESSOR_ARCHITECTURE%.log 2>&1
) else (
@devenv %~dp0\OdbcFb.sln /%BUILDTYPE% "%BUILDCONFIG%|%FB_TARGET_PLATFORM%" /OUT build_%PROCESSOR_ARCHITECTURE%.log 2>&1
)
echo.
)


:: Now make the packages
pushd ..\..\Install\Win32
@echo Now making packages
call MakePackage.bat
popd


goto :EOF

