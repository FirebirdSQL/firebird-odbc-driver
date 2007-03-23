@rem 
@rem Load VCVARS32.BAT from "C:\Program Files\Microsoft Visual Studio 8\VC\bin\"
@rem Create d:/Firebird/include and copy ibase.h iberror.h 
@rem Run this file
@rem
@set COMPDIR=%VCINSTALLDIR%
@"%VCINSTALLDIR%\Bin"\nmake -f makefile.msvc8 %1
