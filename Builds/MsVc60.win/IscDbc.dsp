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
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\Obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(INTERBASE)/include" /I "../.." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"$(INTERBASE)/lib" /EXPORT:createConnection /EXPORT:createServices
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "IscDbc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\Obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "$(INTERBASE)\include" /I "../.." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /D "LOGGING" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"$(INTERBASE)\lib" /EXPORT:createConnection /EXPORT:createServices
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "IscDbc - Win32 Release"
# Name "IscDbc - Win32 Debug"
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\IscDbc\IscDbc.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\IscDbc\Attachment.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\BinaryBlob.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\BinToHexStr.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Blob.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Connection.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\DateTime.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\EnvShare.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscArray.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscBlob.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscCallableStatement.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscColumnKeyInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscColumnPrivilegesResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscColumnsResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscConnection.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscCrossReferenceResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscDatabaseMetaData.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscDbc.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscHeadSqlVar.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscIndexInfoResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscMetaDataResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscOdbcStatement.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscPreparedStatement.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscPrimaryKeysResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscProcedureColumnsResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscProceduresResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscResultSetMetaData.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscSpecialColumnsResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscSqlType.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscStatement.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscStatementMetaData.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscTablePrivilegesResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscTablesResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscUserEvents.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\JavaType.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\JString.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\LinkedList.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\LoadFbClientDll.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Lock.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Mlist.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\MultibyteConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Mutex.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Parameter.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\ParameterEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Parameters.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\ParametersEvents.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Properties.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\ServiceManager.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Sqlda.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\SQLError.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\SQLException.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\SqlTime.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Stream.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\SupportFunctions.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\TimeStamp.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Types.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\TypesResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Value.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\IscDbc\Attachment.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\BinaryBlob.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Blob.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\DateTime.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\EnvShare.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\extodbc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscArray.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscBlob.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscCallableStatement.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscColumnKeyInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscColumnPrivilegesResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscColumnsResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscConnection.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscCrossReferenceResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscDatabaseMetaData.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscIndexInfoResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscMetaDataResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscOdbcStatement.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscPreparedStatement.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscPrimaryKeysResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscProcedureColumnsResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscProceduresResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscResultSetMetaData.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscSpecialColumnsResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscSqlType.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscStatement.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscStatementMetaData.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscTablePrivilegesResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscTablesResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\IscUserEvents.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\JString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\LinkedList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\LoadFbClientDll.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Lock.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\MultibyteConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Mutex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Parameter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\ParameterEvent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Parameters.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\ParametersEvents.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\ServiceManager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Sqlda.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\SQLError.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\SqlTime.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Stream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\SupportFunctions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\TimeStamp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\TypesResultSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Value.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\Values.cpp
# End Source File
# End Group
# End Target
# End Project
