# Microsoft Developer Studio Generated NMAKE File, Based on OdbcJdbcSetup.dsp
!IF "$(CFG)" == ""
CFG=OdbcJdbcSetup - Win32 Debug
!MESSAGE No configuration specified. Defaulting to OdbcJdbcSetup - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "OdbcJdbcSetup - Win32 Release" && "$(CFG)" != "OdbcJdbcSetup - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OdbcJdbcSetup.mak" CFG="OdbcJdbcSetup - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OdbcJdbcSetup - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "OdbcJdbcSetup - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "OdbcJdbcSetup - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\OdbcJdbcSetup.dll"


CLEAN :
	-@erase "$(INTDIR)\DsnDialog.obj"
	-@erase "$(INTDIR)\OdbcJdbcSetup.obj"
	-@erase "$(INTDIR)\OdbcJdbcSetup.pch"
	-@erase "$(INTDIR)\OdbcJdbcSetup.res"
	-@erase "$(INTDIR)\Setup.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\OdbcJdbcSetup.dll"
	-@erase "$(OUTDIR)\OdbcJdbcSetup.exp"
	-@erase "$(OUTDIR)\OdbcJdbcSetup.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fp"$(INTDIR)\OdbcJdbcSetup.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\OdbcJdbcSetup.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\OdbcJdbcSetup.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\OdbcJdbcSetup.pdb" /machine:I386 /def:".\OdbcJdbcSetup.def" /out:"$(OUTDIR)\OdbcJdbcSetup.dll" /implib:"$(OUTDIR)\OdbcJdbcSetup.lib" 
DEF_FILE= \
	".\OdbcJdbcSetup.def"
LINK32_OBJS= \
	"$(INTDIR)\DsnDialog.obj" \
	"$(INTDIR)\OdbcJdbcSetup.obj" \
	"$(INTDIR)\Setup.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\OdbcJdbcSetup.res"

"$(OUTDIR)\OdbcJdbcSetup.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "OdbcJdbcSetup - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\Debug\OdbcJdbcSetup.dll" "$(OUTDIR)\OdbcJdbcSetup.pch"


CLEAN :
	-@erase "$(INTDIR)\DsnDialog.obj"
	-@erase "$(INTDIR)\OdbcJdbcSetup.obj"
	-@erase "$(INTDIR)\OdbcJdbcSetup.pch"
	-@erase "$(INTDIR)\OdbcJdbcSetup.res"
	-@erase "$(INTDIR)\Setup.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\OdbcJdbcSetup.exp"
	-@erase "$(OUTDIR)\OdbcJdbcSetup.lib"
	-@erase "$(OUTDIR)\OdbcJdbcSetup.pdb"
	-@erase "..\Debug\OdbcJdbcSetup.dll"
	-@erase "..\Debug\OdbcJdbcSetup.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /Fp"$(INTDIR)\OdbcJdbcSetup.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\OdbcJdbcSetup.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\OdbcJdbcSetup.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\OdbcJdbcSetup.pdb" /debug /machine:I386 /def:".\OdbcJdbcSetup.def" /out:"..\Debug/OdbcJdbcSetup.dll" /implib:"$(OUTDIR)\OdbcJdbcSetup.lib" /pdbtype:sept 
DEF_FILE= \
	".\OdbcJdbcSetup.def"
LINK32_OBJS= \
	"$(INTDIR)\DsnDialog.obj" \
	"$(INTDIR)\OdbcJdbcSetup.obj" \
	"$(INTDIR)\Setup.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\OdbcJdbcSetup.res"

"..\Debug\OdbcJdbcSetup.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("OdbcJdbcSetup.dep")
!INCLUDE "OdbcJdbcSetup.dep"
!ELSE 
!MESSAGE Warning: cannot find "OdbcJdbcSetup.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "OdbcJdbcSetup - Win32 Release" || "$(CFG)" == "OdbcJdbcSetup - Win32 Debug"
SOURCE=.\DsnDialog.cpp

"$(INTDIR)\DsnDialog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\OdbcJdbcSetup.pch"


SOURCE=.\OdbcJdbcSetup.cpp

"$(INTDIR)\OdbcJdbcSetup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\OdbcJdbcSetup.pch"


SOURCE=.\OdbcJdbcSetup.rc

"$(INTDIR)\OdbcJdbcSetup.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\Setup.cpp

"$(INTDIR)\Setup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\OdbcJdbcSetup.pch"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "OdbcJdbcSetup - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fp"$(INTDIR)\OdbcJdbcSetup.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\OdbcJdbcSetup.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "OdbcJdbcSetup - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /Fp"$(INTDIR)\OdbcJdbcSetup.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\OdbcJdbcSetup.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

