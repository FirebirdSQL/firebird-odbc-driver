# Microsoft Developer Studio Project File - Name="IscDbc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=IscDbc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "IscDbc.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "IscDbc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Release"
# PROP Intermediate_Dir "../Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(INTERBASE)/include" /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /libpath:"$(INTERBASE)/lib"

!ELSEIF  "$(CFG)" == "IscDbc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Debug"
# PROP Intermediate_Dir "../Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "$(INTERBASE)\include" /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /D "LOGGING" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"$(INTERBASE)\lib"

!ENDIF 

# Begin Target

# Name "IscDbc - Win32 Release"
# Name "IscDbc - Win32 Debug"
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\IscDbc.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Attachment.h
# End Source File
# Begin Source File

SOURCE=.\BinaryBlob.h
# End Source File
# Begin Source File

SOURCE=.\BinToHexStr.h
# End Source File
# Begin Source File

SOURCE=.\Blob.h
# End Source File
# Begin Source File

SOURCE=.\Connection.h
# End Source File
# Begin Source File

SOURCE=.\DateTime.h
# End Source File
# Begin Source File

SOURCE=.\Engine.h
# End Source File
# Begin Source File

SOURCE=.\Error.h
# End Source File
# Begin Source File

SOURCE="C:\Program Files\InterBase\SDK\Include\ibase.h"
# End Source File
# Begin Source File

SOURCE=.\IscArray.h
# End Source File
# Begin Source File

SOURCE=.\IscBlob.h
# End Source File
# Begin Source File

SOURCE=.\IscCallableStatement.h
# End Source File
# Begin Source File

SOURCE=.\IscColumnPrivilegesResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscColumnsResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscConnection.h
# End Source File
# Begin Source File

SOURCE=.\IscCrossReferenceResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscDatabaseMetaData.h
# End Source File
# Begin Source File

SOURCE=.\IscDbc.h
# End Source File
# Begin Source File

SOURCE=.\IscIndexInfoResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscMetaDataResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscPreparedStatement.h
# End Source File
# Begin Source File

SOURCE=.\IscPrimaryKeysResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscProcedureColumnsResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscProceduresResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscResultSetMetaData.h
# End Source File
# Begin Source File

SOURCE=.\IscSpecialColumnsResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscSqlType.h
# End Source File
# Begin Source File

SOURCE=.\IscStatement.h
# End Source File
# Begin Source File

SOURCE=.\IscStatementMetaData.h
# End Source File
# Begin Source File

SOURCE=.\IscTablePrivilegesResultSet.h
# End Source File
# Begin Source File

SOURCE=.\IscTablesResultSet.h
# End Source File
# Begin Source File

SOURCE=.\JavaType.h
# End Source File
# Begin Source File

SOURCE=.\JString.h
# End Source File
# Begin Source File

SOURCE=.\LinkedList.h
# End Source File
# Begin Source File

SOURCE=.\LoadFbClientDll.h
# End Source File
# Begin Source File

SOURCE=.\Lock.h
# End Source File
# Begin Source File

SOURCE=.\Mutex.h
# End Source File
# Begin Source File

SOURCE=.\Parameter.h
# End Source File
# Begin Source File

SOURCE=.\Parameters.h
# End Source File
# Begin Source File

SOURCE=..\..\netfrastructure\Engine\Properties.h
# End Source File
# Begin Source File

SOURCE=.\Sqlda.h
# End Source File
# Begin Source File

SOURCE=.\SQLError.h
# End Source File
# Begin Source File

SOURCE=.\SQLException.h
# End Source File
# Begin Source File

SOURCE=.\SqlTime.h
# End Source File
# Begin Source File

SOURCE=.\Stream.h
# End Source File
# Begin Source File

SOURCE=.\TimeStamp.h
# End Source File
# Begin Source File

SOURCE=.\Types.h
# End Source File
# Begin Source File

SOURCE=.\TypesResultSet.h
# End Source File
# Begin Source File

SOURCE=.\Value.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Attachment.cpp
# End Source File
# Begin Source File

SOURCE=.\BinaryBlob.cpp
# End Source File
# Begin Source File

SOURCE=.\Blob.cpp
# End Source File
# Begin Source File

SOURCE=.\DateTime.cpp
# End Source File
# Begin Source File

SOURCE=.\Error.cpp
# End Source File
# Begin Source File

SOURCE=.\extodbc.cpp
# End Source File
# Begin Source File

SOURCE=.\IscArray.cpp
# End Source File
# Begin Source File

SOURCE=.\IscBlob.cpp
# End Source File
# Begin Source File

SOURCE=.\IscCallableStatement.cpp
# End Source File
# Begin Source File

SOURCE=.\IscColumnPrivilegesResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscColumnsResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\IscCrossReferenceResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscDatabaseMetaData.cpp
# End Source File
# Begin Source File

SOURCE=.\IscDbc.def
# End Source File
# Begin Source File

SOURCE=.\IscIndexInfoResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscMetaDataResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscPreparedStatement.cpp
# End Source File
# Begin Source File

SOURCE=.\IscPrimaryKeysResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscProcedureColumnsResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscProceduresResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscResultSetMetaData.cpp
# End Source File
# Begin Source File

SOURCE=.\IscSpecialColumnsResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscSqlType.cpp
# End Source File
# Begin Source File

SOURCE=.\IscStatement.cpp
# End Source File
# Begin Source File

SOURCE=.\IscStatementMetaData.cpp
# End Source File
# Begin Source File

SOURCE=.\IscTablePrivilegesResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\IscTablesResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\JString.cpp
# End Source File
# Begin Source File

SOURCE=.\LinkedList.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadFbClientDll.cpp
# End Source File
# Begin Source File

SOURCE=.\Lock.cpp
# End Source File
# Begin Source File

SOURCE=.\Mutex.cpp
# End Source File
# Begin Source File

SOURCE=.\Parameter.cpp
# End Source File
# Begin Source File

SOURCE=.\Parameters.cpp
# End Source File
# Begin Source File

SOURCE=.\Sqlda.cpp
# End Source File
# Begin Source File

SOURCE=.\SQLError.cpp
# End Source File
# Begin Source File

SOURCE=.\SqlTime.cpp
# End Source File
# Begin Source File

SOURCE=.\Stream.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeStamp.cpp
# End Source File
# Begin Source File

SOURCE=.\TypesResultSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Value.cpp
# End Source File
# Begin Source File

SOURCE=.\Values.cpp
# End Source File
# End Group
# End Target
# End Project
