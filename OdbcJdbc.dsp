# Microsoft Developer Studio Project File - Name="OdbcJdbc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OdbcJdbc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OdbcJdbc.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OdbcJdbc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "IscDbc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "OdbcJdbc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "IscDbc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /D "LOGGING" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 IscDbc.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"debug"
# SUBTRACT LINK32 /incremental:no
# Begin Special Build Tool
TargetDir=.\Debug
SOURCE="$(InputPath)"
PostBuild_Desc=Registering Server
PostBuild_Cmds=regsvr32 /s $(TargetDir)/OdbcJdbcSetup.dll
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "OdbcJdbc - Win32 Release"
# Name "OdbcJdbc - Win32 Debug"
# Begin Group "Resource Files"

# PROP Default_Filter ".rc"
# Begin Source File

SOURCE=.\ODbcJdbc.rc
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DescRecord.cpp
# End Source File
# Begin Source File

SOURCE=.\IscDbc\JString.cpp
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\OdbcConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\OdbcDateTime.cpp
# End Source File
# Begin Source File

SOURCE=.\OdbcDesc.cpp
# End Source File
# Begin Source File

SOURCE=.\OdbcEnv.cpp
# End Source File
# Begin Source File

SOURCE=.\OdbcError.cpp
# End Source File
# Begin Source File

SOURCE=.\OdbcJdbc.def
# End Source File
# Begin Source File

SOURCE=.\OdbcObject.cpp
# End Source File
# Begin Source File

SOURCE=.\OdbcStatement.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\DescRecord.h
# End Source File
# Begin Source File

SOURCE=.\InfoItems.h
# End Source File
# Begin Source File

SOURCE=.\IscDbc\JString.h
# End Source File
# Begin Source File

SOURCE=.\OdbcConnection.h
# End Source File
# Begin Source File

SOURCE=.\OdbcDateTime.h
# End Source File
# Begin Source File

SOURCE=.\OdbcDesc.h
# End Source File
# Begin Source File

SOURCE=.\OdbcEnv.h
# End Source File
# Begin Source File

SOURCE=.\OdbcError.h
# End Source File
# Begin Source File

SOURCE=.\OdbcJdbc.h
# End Source File
# Begin Source File

SOURCE=.\OdbcObject.h
# End Source File
# Begin Source File

SOURCE=.\OdbcStatement.h
# End Source File
# Begin Source File

SOURCE=.\SetupAttributes.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\IscDbc\TimeStamp.h
# End Source File
# End Target
# End Project
