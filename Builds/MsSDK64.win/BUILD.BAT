@rem 
@rem Load D:\WINSDK\SetEnv.Bat /XP64
@rem Create d:/Firebird/include and copy ibase.h iberror.h 
@rem Run this file
@rem
@set COMPDIR=%MSSdk%
@"%MSSdk%\Bin\Win64"\nmake -f makefile.mssdk64 %1
