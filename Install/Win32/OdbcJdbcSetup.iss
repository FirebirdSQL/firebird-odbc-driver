;
;     The contents of this file are subject to the Initial
;     Developer's Public License Version 1.0 (the "License");
;     you may not use this file except in compliance with the
;     License. You may obtain a copy of the License at
;     http://www.ibphoenix.com/idpl.html.
;
;     Software distributed under the License is distributed on
;     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
;     express or implied.  See the License for the specific
;     language governing rights and limitations under the License.
;
;
;  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
;
;  Copyright (c) 2003 Vladimir Tsvigun
;  All Rights Reserved.
;
; OdbcJdbcSetup.iss
;
; for Inno Setup ver: 3.0.6.2 from http://www.innosetup.com/

#define MSVC_VERSION 6
#define BUILD_TYPE "Release"
#ifdef MSVC_VERSION=6
#define BUILD_ENV "MsVc60.win"
;else ;Not yet implemented
;if MSVC_VERSION=7
;#define BUILD_ENV "MsVc70.win"
#endif

#ifdef BUILD_TYPE="Debug"
#define debug_str="_debug"
#else
#define debug_str=""
#endif

#define BUILD_ROOT="..\..\OdbcJdbc"
#define SOURCE_DLL=#BUILD_ROOT\Builds\#BUILD_ENV\#BUILD_TYPE\"
#define SOURCE_CHM="..\..\Install\HtmlHelp\"

#define FIREBIRD_URL "http://www.firebirdsql.org"
#define BASE_VERSION "1_2"


[Setup]
AppName=Firebird ODBC Driver
AppVerName=OdbcJdbc 1.2.0
AppPublisher=Firebird Project
AppPublisherURL={#FIREBIRD_URL}
AppSupportURL={#FIREBIRD_URL}
AppUpdatesURL={#FIREBIRD_URL}

WizardImageFile=firebird-logo1.bmp
WizardImageBackColor=clWhite
WizardSmallImageFile=firebird-logo2.bmp
WizardSmallImageBackColor=clWhite
DefaultDirName={pf}\Firebird\Firebird_ODBC
DefaultGroupName=Firebird\Firebird ODBC Driver
UninstallDisplayIcon={app}\OdbcJdbcSetup.dll
LicenseFile=IDPLicense.txt
InfoAfterFile=Win32\readme.txt

PrivilegesRequired=admin
SourceDir=#BUILD_ROOT
OutputDir=#BUILD_ROOT\Install\Win32\install_image
!OutputBaseFilename=Firebird-1.5.0-Win32{#debug_str}{#pdb_str}
Compression=bzip


[Types]
Name: NormalInstall; Description: "Install and register driver in System Dir. Install documentation to program group."
Name: LocalInstall: Description: "Install driver locally. Do not register."
Name: DocumentationInstall: Description: "Install documentation only."
Name: MinumumInstall; Description: "Minimum install - no docs, no menus, no icons, no uninstall." Flags: disablenouninstallwarning;
;Name: CustomInstall; Description: "Custom installation"; Flags: iscustom;

[Components]
Name: SystemLibraryComponent; Description: Driver Library; Types: FullInstall MinimumInstall;
Name: LocalLibraryComponent; Description: Driver Library; Types: LocalInstall;
Name: DocumentationComponent; Description: Documenation; Types: FullInstall LocalInstall DocumentationInstall;

;[Tasks]
;Name: RetainLibrariesLocally; Description: "Retain local copies of driver libraries? (In addition to installing into System Directory.)" ; MinVersion: 4.0,4.0; Check: ConfigureFirebird;
;Name: NoRegister; Description: "Do not register driver." ; MinVersion: 4.0,4.0; Check: ConfigureFirebird;

[Files]

Source: "{#SOURCE_DLL}IscDbc.dll"; DestDir: "{app}"; Components: SystemLibraryComponent Flags: ignoreversion deleteafterinstall
Source: "{#SOURCE_DLL}OdbcJdbc.dll"; DestDir: "{app}"; Components: SystemLibraryComponent Flags: ignoreversion deleteafterinstall
Source: "{#SOURCE_DLL}OdbcJdbcSetup.dll"; DestDir: "{app}"; Components: SystemLibraryComponent Flags: ignoreversion
Source: "{#SOURCE_CHM}OdbcJdbc.chm"; DestDir: "{sys}"; Flags: ignoreversion
Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme
Source: "Win32\IDPLicense.txt"; DestDir: "{app}";

[Icons]
Name: "{group}\Firebird ODBC"; Filename: "{uninstallexe}"
Name: "{group}\Firebird ODBC Help"; Filename: "{sys}\OdbcJdbc.chm"
Name: "{group}\Firebird ODBC v1.2 Release Notes"; Filename: "{app}\ReleaseNotes_v1.2.html"
Name: "{group}\Firebird ODBC readme.txt"; Filename: "{app}\Readme.txt"
Name: "{group}\Firebird ODBC license.txt"; Filename: "{app}\IDPLicense.txt"


[Run]
Filename: "{sys}\regsvr32.exe"; Parameters: "/s ""{app}""\OdbcJdbcSetup.dll"

[UninstallRun]
Filename: "{sys}\regsvr32.exe"; Parameters: "/u/s ""{app}""\OdbcJdbcSetup.dll"




