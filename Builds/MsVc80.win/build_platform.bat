
:BUILD
::========
@call setenvvar.bat %1 %2
@title %BUILDTYPE%  %1%
@echo   %BUILDTYPE% %1%
@set > build_%1%.log
if defined USE_NMAKE (
@echo using NMAKE
@nmake /i /f %~dp0\makefile.%vs_ver% %1 all >> build_%1%.log 2>&1
) else (
@devenv %~dp0\OdbcFb.sln /%BUILDTYPE% "%BUILDCONFIG%|%FB_TARGET_PLATFORM%" /OUT build_%1%.log 2>&1
)
echo.
)


:: Now make the packages
pushd ..\..\Install\Win32
@echo Now making packages
call MakePackage.bat
popd


goto :EOF

