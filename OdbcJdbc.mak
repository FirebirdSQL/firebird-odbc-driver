# Microsoft Developer Studio Generated NMAKE File, Based on OdbcJdbc.dsp
!IF "$(CFG)" == ""
CFG=OdbcJdbc - Win32 Debug
!MESSAGE No configuration specified. Defaulting to OdbcJdbc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "OdbcJdbc - Win32 Release" && "$(CFG)" != "OdbcJdbc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OdbcJdbc.mak" CFG="OdbcJdbc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OdbcJdbc - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "OdbcJdbc - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OdbcJdbc - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\OdbcJdbc.dll"

!ELSE 

ALL : "IscDbc - Win32 Release" "$(OUTDIR)\OdbcJdbc.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"IscDbc - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\DescRecord.obj"
	-@erase "$(INTDIR)\IscDbcTime.obj"
	-@erase "$(INTDIR)\JString.obj"
	-@erase "$(INTDIR)\Main.obj"
	-@erase "$(INTDIR)\OdbcConnection.obj"
	-@erase "$(INTDIR)\OdbcDateTime.obj"
	-@erase "$(INTDIR)\OdbcDesc.obj"
	-@erase "$(INTDIR)\OdbcEnv.obj"
	-@erase "$(INTDIR)\OdbcError.obj"
	-@erase "$(INTDIR)\OdbcObject.obj"
	-@erase "$(INTDIR)\OdbcStatement.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\OdbcJdbc.dll"
	-@erase "$(OUTDIR)\OdbcJdbc.exp"
	-@erase "$(OUTDIR)\OdbcJdbc.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "IscDbc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\OdbcJdbc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\OdbcJdbc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\OdbcJdbc.pdb" /machine:I386 /def:".\OdbcJdbc.def" /out:"$(OUTDIR)\OdbcJdbc.dll" /implib:"$(OUTDIR)\OdbcJdbc.lib" 
DEF_FILE= \
	".\OdbcJdbc.def"
LINK32_OBJS= \
	"$(INTDIR)\DescRecord.obj" \
	"$(INTDIR)\JString.obj" \
	"$(INTDIR)\Main.obj" \
	"$(INTDIR)\OdbcConnection.obj" \
	"$(INTDIR)\OdbcDateTime.obj" \
	"$(INTDIR)\OdbcDesc.obj" \
	"$(INTDIR)\OdbcEnv.obj" \
	"$(INTDIR)\OdbcError.obj" \
	"$(INTDIR)\OdbcObject.obj" \
	"$(INTDIR)\OdbcStatement.obj" \
	"$(INTDIR)\IscDbcTime.obj" \
	".\IscDbc\Release\IscDbc.lib"

"$(OUTDIR)\OdbcJdbc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "OdbcJdbc - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\OdbcJdbc.dll"

!ELSE 

ALL : "IscDbc - Win32 Debug" "$(OUTDIR)\OdbcJdbc.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"IscDbc - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\DescRecord.obj"
	-@erase "$(INTDIR)\IscDbcTime.obj"
	-@erase "$(INTDIR)\JString.obj"
	-@erase "$(INTDIR)\Main.obj"
	-@erase "$(INTDIR)\OdbcConnection.obj"
	-@erase "$(INTDIR)\OdbcDateTime.obj"
	-@erase "$(INTDIR)\OdbcDesc.obj"
	-@erase "$(INTDIR)\OdbcEnv.obj"
	-@erase "$(INTDIR)\OdbcError.obj"
	-@erase "$(INTDIR)\OdbcObject.obj"
	-@erase "$(INTDIR)\OdbcStatement.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\OdbcJdbc.dll"
	-@erase "$(OUTDIR)\OdbcJdbc.exp"
	-@erase "$(OUTDIR)\OdbcJdbc.ilk"
	-@erase "$(OUTDIR)\OdbcJdbc.lib"
	-@erase "$(OUTDIR)\OdbcJdbc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "IscDbc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /D "LOGGING" /Fp"$(INTDIR)\OdbcJdbc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\OdbcJdbc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=IscDbc.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\OdbcJdbc.pdb" /debug /machine:I386 /def:".\OdbcJdbc.def" /out:"$(OUTDIR)\OdbcJdbc.dll" /implib:"$(OUTDIR)\OdbcJdbc.lib" /pdbtype:sept /libpath:"IscDbc\Debug" 
DEF_FILE= \
	".\OdbcJdbc.def"
LINK32_OBJS= \
	"$(INTDIR)\DescRecord.obj" \
	"$(INTDIR)\JString.obj" \
	"$(INTDIR)\Main.obj" \
	"$(INTDIR)\OdbcConnection.obj" \
	"$(INTDIR)\OdbcDateTime.obj" \
	"$(INTDIR)\OdbcDesc.obj" \
	"$(INTDIR)\OdbcEnv.obj" \
	"$(INTDIR)\OdbcError.obj" \
	"$(INTDIR)\OdbcObject.obj" \
	"$(INTDIR)\OdbcStatement.obj" \
	"$(INTDIR)\IscDbcTime.obj" \
	".\IscDbc\Debug\IscDbc.lib"

"$(OUTDIR)\OdbcJdbc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("OdbcJdbc.dep")
!INCLUDE "OdbcJdbc.dep"
!ELSE 
!MESSAGE Warning: cannot find "OdbcJdbc.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "OdbcJdbc - Win32 Release" || "$(CFG)" == "OdbcJdbc - Win32 Debug"

!IF  "$(CFG)" == "OdbcJdbc - Win32 Release"

"IscDbc - Win32 Release" : 
   cd ".\IscDbc"
   $(MAKE) /$(MAKEFLAGS) /F .\IscDbc.mak CFG="IscDbc - Win32 Release" 
   cd ".."

"IscDbc - Win32 ReleaseCLEAN" : 
   cd ".\IscDbc"
   $(MAKE) /$(MAKEFLAGS) /F .\IscDbc.mak CFG="IscDbc - Win32 Release" RECURSE=1 CLEAN 
   cd ".."

!ELSEIF  "$(CFG)" == "OdbcJdbc - Win32 Debug"

"IscDbc - Win32 Debug" : 
   cd ".\IscDbc"
   $(MAKE) /$(MAKEFLAGS) /F .\IscDbc.mak CFG="IscDbc - Win32 Debug" 
   cd ".."

"IscDbc - Win32 DebugCLEAN" : 
   cd ".\IscDbc"
   $(MAKE) /$(MAKEFLAGS) /F .\IscDbc.mak CFG="IscDbc - Win32 Debug" RECURSE=1 CLEAN 
   cd ".."

!ENDIF 

SOURCE=.\DescRecord.cpp

"$(INTDIR)\DescRecord.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscDbc\IscDbcTime.cpp

"$(INTDIR)\IscDbcTime.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\IscDbc\JString.cpp

"$(INTDIR)\JString.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Main.cpp

"$(INTDIR)\Main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OdbcConnection.cpp

"$(INTDIR)\OdbcConnection.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OdbcDateTime.cpp

"$(INTDIR)\OdbcDateTime.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OdbcDesc.cpp

"$(INTDIR)\OdbcDesc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OdbcEnv.cpp

"$(INTDIR)\OdbcEnv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OdbcError.cpp

"$(INTDIR)\OdbcError.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OdbcObject.cpp

"$(INTDIR)\OdbcObject.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OdbcStatement.cpp

"$(INTDIR)\OdbcStatement.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

