@rem 
@rem Load VCVARS32.BAT from "D:\Program Files\Microsoft Visual Studio .NET\Vc7\bin"
@rem Create d:/Firebird/include and copy ibase.h iberror.h 
@rem Run this file
@rem
@set COMPDIR=%MSVCDir%
@"%MSVCDir%\Bin"\nmake -f makefile.msvc7 %1
