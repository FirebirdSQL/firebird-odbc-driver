# Microsoft Developer Studio Generated NMAKE File, Based on IscDbc.dsp
!IF "$(CFG)" == ""
CFG=IscDbc - Win32 Debug
!MESSAGE No configuration specified. Defaulting to IscDbc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "IscDbc - Win32 Release" && "$(CFG)" != "IscDbc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "IscDbc.mak" CFG="IscDbc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IscDbc - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IscDbc - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "IscDbc - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\IscDbc.dll"


CLEAN :
	-@erase "$(INTDIR)\AsciiBlob.obj"
	-@erase "$(INTDIR)\Attachment.obj"
	-@erase "$(INTDIR)\BinaryBlob.obj"
	-@erase "$(INTDIR)\Blob.obj"
	-@erase "$(INTDIR)\DateTime.obj"
	-@erase "$(INTDIR)\Error.obj"
	-@erase "$(INTDIR)\IscBlob.obj"
	-@erase "$(INTDIR)\IscCallableStatement.obj"
	-@erase "$(INTDIR)\IscColumnsResultSet.obj"
	-@erase "$(INTDIR)\IscConnection.obj"
	-@erase "$(INTDIR)\IscCrossReferenceResultSet.obj"
	-@erase "$(INTDIR)\IscDatabaseMetaData.obj"
	-@erase "$(INTDIR)\IscIndexInfoResultSet.obj"
	-@erase "$(INTDIR)\IscMetaDataResultSet.obj"
	-@erase "$(INTDIR)\IscPreparedStatement.obj"
	-@erase "$(INTDIR)\IscPrimaryKeysResultSet.obj"
	-@erase "$(INTDIR)\IscProcedureColumnsResultSet.obj"
	-@erase "$(INTDIR)\IscProceduresResultSet.obj"
	-@erase "$(INTDIR)\IscResultSet.obj"
	-@erase "$(INTDIR)\IscResultSetMetaData.obj"
	-@erase "$(INTDIR)\IscSqlType.obj"
	-@erase "$(INTDIR)\IscStatement.obj"
	-@erase "$(INTDIR)\IscStatementMetaData.obj"
	-@erase "$(INTDIR)\IscTablesResultSet.obj"
	-@erase "$(INTDIR)\JString.obj"
	-@erase "$(INTDIR)\LinkedList.obj"
	-@erase "$(INTDIR)\Lock.obj"
	-@erase "$(INTDIR)\Mutex.obj"
	-@erase "$(INTDIR)\Parameter.obj"
	-@erase "$(INTDIR)\Parameters.obj"
	-@erase "$(INTDIR)\Sqlda.obj"
	-@erase "$(INTDIR)\SQLError.obj"
	-@erase "$(INTDIR)\Stream.obj"
	-@erase "$(INTDIR)\TimeStamp.obj"
	-@erase "$(INTDIR)\Value.obj"
	-@erase "$(INTDIR)\Values.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\IscDbc.dll"
	-@erase "$(OUTDIR)\IscDbc.exp"
	-@erase "$(OUTDIR)\IscDbc.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\IscDbc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\IscDbc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\IscDbc.pdb" /machine:I386 /def:".\IscDbc.def" /out:"$(OUTDIR)\IscDbc.dll" /implib:"$(OUTDIR)\IscDbc.lib" 
DEF_FILE= \
	".\IscDbc.def"
LINK32_OBJS= \
	"$(INTDIR)\AsciiBlob.obj" \
	"$(INTDIR)\Attachment.obj" \
	"$(INTDIR)\BinaryBlob.obj" \
	"$(INTDIR)\Blob.obj" \
	"$(INTDIR)\DateTime.obj" \
	"$(INTDIR)\Error.obj" \
	"$(INTDIR)\IscBlob.obj" \
	"$(INTDIR)\IscCallableStatement.obj" \
	"$(INTDIR)\IscColumnsResultSet.obj" \
	"$(INTDIR)\IscConnection.obj" \
	"$(INTDIR)\IscCrossReferenceResultSet.obj" \
	"$(INTDIR)\IscDatabaseMetaData.obj" \
	"$(INTDIR)\IscIndexInfoResultSet.obj" \
	"$(INTDIR)\IscMetaDataResultSet.obj" \
	"$(INTDIR)\IscPreparedStatement.obj" \
	"$(INTDIR)\IscPrimaryKeysResultSet.obj" \
	"$(INTDIR)\IscProcedureColumnsResultSet.obj" \
	"$(INTDIR)\IscProceduresResultSet.obj" \
	"$(INTDIR)\IscResultSet.obj" \
	"$(INTDIR)\IscResultSetMetaData.obj" \
	"$(INTDIR)\IscSqlType.obj" \
	"$(INTDIR)\IscStatement.obj" \
	"$(INTDIR)\IscStatementMetaData.obj" \
	"$(INTDIR)\IscTablesResultSet.obj" \
	"$(INTDIR)\JString.obj" \
	"$(INTDIR)\LinkedList.obj" \
	"$(INTDIR)\Lock.obj" \
	"$(INTDIR)\Mutex.obj" \
	"$(INTDIR)\Parameter.obj" \
	"$(INTDIR)\Parameters.obj" \
	"$(INTDIR)\Sqlda.obj" \
	"$(INTDIR)\SQLError.obj" \
	"$(INTDIR)\Stream.obj" \
	"$(INTDIR)\TimeStamp.obj" \
	"$(INTDIR)\Value.obj" \
	"$(INTDIR)\Values.obj"

"$(OUTDIR)\IscDbc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "IscDbc - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\Debug\IscDbc.dll"


CLEAN :
	-@erase "$(INTDIR)\AsciiBlob.obj"
	-@erase "$(INTDIR)\Attachment.obj"
	-@erase "$(INTDIR)\BinaryBlob.obj"
	-@erase "$(INTDIR)\Blob.obj"
	-@erase "$(INTDIR)\DateTime.obj"
	-@erase "$(INTDIR)\Error.obj"
	-@erase "$(INTDIR)\IscBlob.obj"
	-@erase "$(INTDIR)\IscCallableStatement.obj"
	-@erase "$(INTDIR)\IscColumnsResultSet.obj"
	-@erase "$(INTDIR)\IscConnection.obj"
	-@erase "$(INTDIR)\IscCrossReferenceResultSet.obj"
	-@erase "$(INTDIR)\IscDatabaseMetaData.obj"
	-@erase "$(INTDIR)\IscIndexInfoResultSet.obj"
	-@erase "$(INTDIR)\IscMetaDataResultSet.obj"
	-@erase "$(INTDIR)\IscPreparedStatement.obj"
	-@erase "$(INTDIR)\IscPrimaryKeysResultSet.obj"
	-@erase "$(INTDIR)\IscProcedureColumnsResultSet.obj"
	-@erase "$(INTDIR)\IscProceduresResultSet.obj"
	-@erase "$(INTDIR)\IscResultSet.obj"
	-@erase "$(INTDIR)\IscResultSetMetaData.obj"
	-@erase "$(INTDIR)\IscSqlType.obj"
	-@erase "$(INTDIR)\IscStatement.obj"
	-@erase "$(INTDIR)\IscStatementMetaData.obj"
	-@erase "$(INTDIR)\IscTablesResultSet.obj"
	-@erase "$(INTDIR)\JString.obj"
	-@erase "$(INTDIR)\LinkedList.obj"
	-@erase "$(INTDIR)\Lock.obj"
	-@erase "$(INTDIR)\Mutex.obj"
	-@erase "$(INTDIR)\Parameter.obj"
	-@erase "$(INTDIR)\Parameters.obj"
	-@erase "$(INTDIR)\Sqlda.obj"
	-@erase "$(INTDIR)\SQLError.obj"
	-@erase "$(INTDIR)\Stream.obj"
	-@erase "$(INTDIR)\TimeStamp.obj"
	-@erase "$(INTDIR)\Value.obj"
	-@erase "$(INTDIR)\Values.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\IscDbc.exp"
	-@erase "$(OUTDIR)\IscDbc.lib"
	-@erase "$(OUTDIR)\IscDbc.pdb"
	-@erase "..\Debug\IscDbc.dll"
	-@erase "..\Debug\IscDbc.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "c:\Program Files\Borland\Interbase\SDK\include" /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\IscDbc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\IscDbc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=gds32_ms.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\IscDbc.pdb" /debug /machine:I386 /def:".\IscDbc.def" /out:"..\Debug/IscDbc.dll" /implib:"$(OUTDIR)\IscDbc.lib" /pdbtype:sept /libpath:"c:\Program Files\Borland\Interbase\SDK\lib_ms" 
DEF_FILE= \
	".\IscDbc.def"
LINK32_OBJS= \
	"$(INTDIR)\AsciiBlob.obj" \
	"$(INTDIR)\Attachment.obj" \
	"$(INTDIR)\BinaryBlob.obj" \
	"$(INTDIR)\Blob.obj" \
	"$(INTDIR)\DateTime.obj" \
	"$(INTDIR)\Error.obj" \
	"$(INTDIR)\IscBlob.obj" \
	"$(INTDIR)\IscCallableStatement.obj" \
	"$(INTDIR)\IscColumnsResultSet.obj" \
	"$(INTDIR)\IscConnection.obj" \
	"$(INTDIR)\IscCrossReferenceResultSet.obj" \
	"$(INTDIR)\IscDatabaseMetaData.obj" \
	"$(INTDIR)\IscIndexInfoResultSet.obj" \
	"$(INTDIR)\IscMetaDataResultSet.obj" \
	"$(INTDIR)\IscPreparedStatement.obj" \
	"$(INTDIR)\IscPrimaryKeysResultSet.obj" \
	"$(INTDIR)\IscProcedureColumnsResultSet.obj" \
	"$(INTDIR)\IscProceduresResultSet.obj" \
	"$(INTDIR)\IscResultSet.obj" \
	"$(INTDIR)\IscResultSetMetaData.obj" \
	"$(INTDIR)\IscSqlType.obj" \
	"$(INTDIR)\IscStatement.obj" \
	"$(INTDIR)\IscStatementMetaData.obj" \
	"$(INTDIR)\IscTablesResultSet.obj" \
	"$(INTDIR)\JString.obj" \
	"$(INTDIR)\LinkedList.obj" \
	"$(INTDIR)\Lock.obj" \
	"$(INTDIR)\Mutex.obj" \
	"$(INTDIR)\Parameter.obj" \
	"$(INTDIR)\Parameters.obj" \
	"$(INTDIR)\Sqlda.obj" \
	"$(INTDIR)\SQLError.obj" \
	"$(INTDIR)\Stream.obj" \
	"$(INTDIR)\TimeStamp.obj" \
	"$(INTDIR)\Value.obj" \
	"$(INTDIR)\Values.obj"

"..\Debug\IscDbc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("IscDbc.dep")
!INCLUDE "IscDbc.dep"
!ELSE 
!MESSAGE Warning: cannot find "IscDbc.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "IscDbc - Win32 Release" || "$(CFG)" == "IscDbc - Win32 Debug"
SOURCE=.\AsciiBlob.cpp

"$(INTDIR)\AsciiBlob.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Attachment.cpp

"$(INTDIR)\Attachment.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\BinaryBlob.cpp

"$(INTDIR)\BinaryBlob.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\netfrastructure\Engine\Blob.cpp

"$(INTDIR)\Blob.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\DateTime.cpp

"$(INTDIR)\DateTime.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Error.cpp

"$(INTDIR)\Error.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscBlob.cpp

"$(INTDIR)\IscBlob.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscCallableStatement.cpp

"$(INTDIR)\IscCallableStatement.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscColumnsResultSet.cpp

"$(INTDIR)\IscColumnsResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscConnection.cpp

"$(INTDIR)\IscConnection.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscCrossReferenceResultSet.cpp

"$(INTDIR)\IscCrossReferenceResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscDatabaseMetaData.cpp

"$(INTDIR)\IscDatabaseMetaData.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscIndexInfoResultSet.cpp

"$(INTDIR)\IscIndexInfoResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscMetaDataResultSet.cpp

"$(INTDIR)\IscMetaDataResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscPreparedStatement.cpp

"$(INTDIR)\IscPreparedStatement.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscPrimaryKeysResultSet.cpp

"$(INTDIR)\IscPrimaryKeysResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscProcedureColumnsResultSet.cpp

"$(INTDIR)\IscProcedureColumnsResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscProceduresResultSet.cpp

"$(INTDIR)\IscProceduresResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscResultSet.cpp

"$(INTDIR)\IscResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscResultSetMetaData.cpp

"$(INTDIR)\IscResultSetMetaData.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscSqlType.cpp

"$(INTDIR)\IscSqlType.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscStatement.cpp

"$(INTDIR)\IscStatement.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscStatementMetaData.cpp

"$(INTDIR)\IscStatementMetaData.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IscTablesResultSet.cpp

"$(INTDIR)\IscTablesResultSet.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\JString.cpp

"$(INTDIR)\JString.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\LinkedList.cpp

"$(INTDIR)\LinkedList.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Lock.cpp

"$(INTDIR)\Lock.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Mutex.cpp

"$(INTDIR)\Mutex.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Parameter.cpp

"$(INTDIR)\Parameter.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Parameters.cpp

"$(INTDIR)\Parameters.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Sqlda.cpp

"$(INTDIR)\Sqlda.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SQLError.cpp

"$(INTDIR)\SQLError.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Stream.cpp

"$(INTDIR)\Stream.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\TimeStamp.cpp

"$(INTDIR)\TimeStamp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Value.cpp

"$(INTDIR)\Value.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Values.cpp

"$(INTDIR)\Values.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

