@call ..\delDependMT.bat
@"D:\Program Files\VS6\VC98\Bin"\nmake -f makefile.msvc6
@call ..\delDependMT.bat
@"D:\Program Files\VS6\VC98\Bin"\nmake -f makefile.msvc6 MTDLL=1
