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
# PROP Intermediate_Dir "Release\Obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../IscDbc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__MONITOR_EXECUTING" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 version.lib gdi32.lib shell32.lib user32.lib odbccp32.lib comdlg32.lib comctl32.lib wsock32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"Release/OdbcFb32.dll" /EXPORT:ConfigDSN /EXPORT:ConfigDriver,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE /EXPORT:DllInstall,PRIVATE
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "OdbcJdbc - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../IscDbc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /D "LOGGING" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib gdi32.lib shell32.lib user32.lib odbccp32.lib comdlg32.lib comctl32.lib wsock32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug/OdbcFb32.dll" /pdbtype:sept /libpath:"debug" /EXPORT:ConfigDSN /EXPORT:ConfigDriver,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE /EXPORT:DllInstall,PRIVATE /EXPORT:createConnection
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "OdbcJdbc - Win32 Release"
# Name "OdbcJdbc - Win32 Debug"
# Begin Group "IscDbc"

# PROP Default_Filter "cpp;h;"
# Begin Group "Source Files IscDbc"

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
# Begin Group "Header Files IscDbc"

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
# Begin Group "Resource Files IscDbc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\IscDbc\IscDbc.rc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Group
# Begin Group "OdbcJdbc"

# PROP Default_Filter "cpp;h;"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\ConnectDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\DescRecord.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\MainUnicode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\MbsAndWcs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcConnection.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcDateTime.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcDesc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcEnv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcError.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbc.def
# End Source File
# Begin Source File

SOURCE=..\..\OdbcObject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcStatement.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ResourceManagerSink.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SafeEnvThread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\TransactionResourceAsync.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\ConnectDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\DescRecord.h
# End Source File
# Begin Source File

SOURCE=..\..\InfoItems.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcConnection.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcDateTime.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcDesc.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcEnv.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcError.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbc.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcObject.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcStatement.h
# End Source File
# Begin Source File

SOURCE=..\..\Headers\OdbcUserEvents.h
# End Source File
# Begin Source File

SOURCE=..\..\ResourceManagerSink.h
# End Source File
# Begin Source File

SOURCE=..\..\SafeEnvThread.h
# End Source File
# Begin Source File

SOURCE=..\..\TemplateConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\TransactionResourceAsync.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ".rc"
# Begin Source File

SOURCE=..\..\ODbcJdbc.rc
# End Source File
# End Group
# End Group
# Begin Group "OdbcJdbcSetup"

# PROP Default_Filter "cpp;h;"
# Begin Group "Source Files Setup"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\CommonUtil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\DsnDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\OdbcJdbcSetup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\OdbcJdbcSetup.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceClient.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabBackup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabChild.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabCtrl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabRepair.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabRestore.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabStatistics.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabUsers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\Setup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UserDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UsersTabChild.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UsersTabMemberShips.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UsersTabRoles.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UsersTabUsers.cpp
# End Source File
# End Group
# Begin Group "Header Files Setup"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\CommonUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\DsnDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\OdbcJdbcSetup.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\Resource.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceClient.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabBackup.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabChild.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabCtrl.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabRepair.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabRestore.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabStatistics.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceTabUsers.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\Setup.h
# End Source File
# Begin Source File

SOURCE=..\..\SetupAttributes.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UserDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UsersTabChild.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UsersTabMemberShips.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UsersTabRoles.h
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\UsersTabUsers.h
# End Source File
# End Group
# Begin Group "Resource Files Setup"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Group
# End Target
# End Project
