# Microsoft Developer Studio Project File - Name="OdbcJdbcSetup" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OdbcJdbcSetup - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OdbcJdbcSetup.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OdbcJdbcSetup - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\Obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../.." /I "../../IscDbc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "ISOLATION_AWARE_ENABLED" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 version.lib gdi32.lib shell32.lib advapi32.lib user32.lib comdlg32.lib comctl32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /EXPORT:ConfigDSN /EXPORT:ConfigDriver,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE /EXPORT:DllInstall,PRIVATE
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "OdbcJdbcSetup - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\Obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../.." /I "../../IscDbc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "ISOLATION_AWARE_ENABLED" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib gdi32.lib shell32.lib advapi32.lib user32.lib comdlg32.lib comctl32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /EXPORT:ConfigDSN /EXPORT:ConfigDriver,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE /EXPORT:DllInstall,PRIVATE
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "OdbcJdbcSetup - Win32 Release"
# Name "OdbcJdbcSetup - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\DsnDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\IscDbc\JString.cpp

!IF  "$(CFG)" == "OdbcJdbcSetup - Win32 Release"

!ELSEIF  "$(CFG)" == "OdbcJdbcSetup - Win32 Debug"

# SUBTRACT CPP /Fr

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\OdbcJdbcSetup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\OdbcJdbcSetup.rc
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\ServiceClient.cpp
# End Source File
# Begin Source File

SOURCE=..\..\OdbcJdbcSetup\CommonUtil.cpp
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
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
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

SOURCE=..\..\OdbcJdbcSetup\CommonUtil.h
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
# Begin Source File

SOURCE=..\..\SetupAttributes.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\..\change.log
# End Source File
# End Target
# End Project
