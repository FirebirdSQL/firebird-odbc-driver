@rem 
@rem Load VCVARS32.BAT from "C:\Program Files\Microsoft Visual Studio\VC98\Bin"
@rem Create d:/Firebird/include and copy ibase.h iberror.h 
@rem Run this file
@rem
@set COMPDIR=%MSVCDir%
@set COMPDIRDEV=%MSDevDir%
@"%MSVCDir%\Bin"\nmake -f makefile.msvc6 %1
