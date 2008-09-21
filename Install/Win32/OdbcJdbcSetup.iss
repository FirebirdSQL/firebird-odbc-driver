;
;     The contents of this file are subject to the Initial
;     Developer's Public License Version 1.0 (the "License");
;     you may not use this file except in compliance with the
;     License. You may obtain a copy of the License at
;        http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
;
;     Software distributed under the License is distributed on
;     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
;     express or implied.  See the License for the specific
;     language governing rights and limitations under the License.
;
;
;  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
;  Updated and extended by Paul Reeves for v1.2 release.
;
;
;  Copyright (c) 2003 Vladimir Tsvigun
;  Portions Copyright (c) 2004 Paul Reeves
;  All Rights Reserved.
;
;
;
;  OdbcJdbcSetup.iss
;
;  Currently compiled against InnoSetup v4.2.0 from http://www.innosetup.com/
;
;

#define MSVC_VERSION 6
#define BUILD_TYPE "Release"

#if MSVC_VERSION==6
#define BUILD_ENV "MsVc60.win"
#elif MSVC_VERSION==7
#define BUILD_ENV "MsVc70.win"
#else
BUILD_ENV undefined
#endif

#if BUILD_TYPE=="Debug"
#define debug_str "_debug"
#else
#define debug_str ""
#endif

#define FIREBIRD_URL "http://www.firebirdsql.org"

#define BUILD_ROOT="..\..\"
#define SOURCE_LIBS "Builds\"+AddBackslash(BUILD_ENV)+AddBackslash(BUILD_TYPE)
#define SOURCE_DOCS="Install\"


[Setup]
AppName=Firebird ODBC Driver
AppVerName=Firebird/InterBase(r) ODBC driver 2.0
AppVersion=2.0
AppMutex=InnoSetupExtensionsCompilerAppMutex
AppPublisher=Firebird Project
AppPublisherURL={#FIREBIRD_URL}
AppSupportURL={#FIREBIRD_URL}
AppUpdatesURL={#FIREBIRD_URL}

DefaultDirName={pf}\Firebird\Firebird_ODBC
DefaultGroupName=Firebird\Firebird ODBC Driver
UninstallDisplayIcon={sys}\OdbcFb.dll
UninstallFilesDir={localappdata}\OdbcFb

PrivilegesRequired=admin

SourceDir={#BUILD_ROOT}
OutputDir={#SOURCE_DOCS}\Win32\install_image
OutputBaseFilename=Firebird_ODBC_2.0.0-Win32{#debug_str}
DiskSpanning=no

LicenseFile={#SOURCE_DOCS}\IDPLicense.txt
InfoBeforeFile={#SOURCE_DOCS}\Win32\installation_readme.txt
InfoAfterFile={#SOURCE_DOCS}\Win32\readme.txt
Compression=lzma
SolidCompression=true

WizardImageFile={#SOURCE_DOCS}\Win32\firebird-logo1.bmp
WizardImageBackColor=clWhite
WizardSmallImageFile={#SOURCE_DOCS}\Win32\firebird-logo2.bmp


[Languages]
Name: en; MessagesFile: compiler:Default.isl
Name: ru; MessagesFile: compiler:Default.isl,compiler:Languages\Russian.isl


[Types]
Name: DeveloperInstall; Description: {cm:DeveloperInstall}
Name: DeploymentInstall; Description: {cm:DeploymentInstall}
Name: DocumentationInstall; Description: {cm:DocumentationInstall}


[Components]
Name: DeveloperComponent; Description: {cm:DeveloperComponent} {sys}; Types: DeveloperInstall; Flags: exclusive
Name: DeploymentComponent; Description: {cm:DeploymentComponent}; Types: DeploymentInstall; Flags: exclusive disablenouninstallwarning
Name: DocumentationComponent; Description: {cm:DocumentationComponent}; Types: DeveloperInstall DocumentationInstall


[Files]
Source: {#SOURCE_LIBS}OdbcFb.dll; DestDir: {sys}; Components: DeveloperComponent DeploymentComponent; Flags: regserver restartreplace sharedfile
Source: {#SOURCE_DOCS}\HtmlHelp\OdbcFb.chm; DestDir: {app}; Components: DeveloperComponent DeploymentComponent; Flags: ignoreversion
Source: {#SOURCE_DOCS}\HtmlHelp\OdbcFb.chm; DestDir: {sys}; Components: DeveloperComponent DeploymentComponent; Flags: ignoreversion
Source: {#SOURCE_DOCS}\HtmlHelp\OdbcFb.chm; DestDir: {app}; Components: DocumentationComponent; Flags: ignoreversion
Source: {#SOURCE_DOCS}\HtmlHelp\OdbcFb.chm; DestDir: {sys}; Components: DocumentationComponent; Flags: ignoreversion
Source: {#SOURCE_DOCS}\HtmlHelp\html\*.*; DestDir: {app}\html; Components: DocumentationComponent
Source: {#SOURCE_DOCS}\HtmlHelp\images\*.*; DestDir: {app}\images; Components: DocumentationComponent
Source: {#SOURCE_DOCS}\Win32\Readme.txt; DestDir: {app}; Components: DocumentationComponent; Flags: isreadme
Source: {#SOURCE_DOCS}\IDPLicense.txt; DestDir: {app}; Components: DocumentationComponent
;Source: {#SOURCE_DOCS}\ReleaseNotes_v1.2.html; DestDir: {app}; Components: DocumentationComponent


[Icons]
Name: {group}\Uninstall Firebird ODBC driver; Filename: {uninstallexe}; Components: DocumentationComponent; Comment: Remove Firebird ODBC Driver Documentation
Name: {group}\Uninstall Firebird ODBC driver; Filename: {uninstallexe}; Components: DeveloperComponent; Comment: Remove Firebird ODBC Driver Library and Documentation
Name: {group}\Firebird ODBC Help; Filename: {app}\OdbcFb.chm; Components: DocumentationComponent
Name: {group}\Firebird ODBC Help; Filename: {sys}\OdbcFb.chm; Components: DeveloperComponent
Name: {app}\Firebird ODBC Help; Filename: {sys}\OdbcFb.chm; Components: DeveloperComponent
;Name: {group}\Firebird ODBC v1.2 Release Notes; Filename: {app}\ReleaseNotes_v1.2.html; Components: DocumentationComponent
Name: {group}\Firebird ODBC readme.txt; Filename: {app}\Readme.txt; Components: DocumentationComponent
Name: {group}\Firebird ODBC license.txt; Filename: {app}\IDPLicense.txt; Components: DocumentationComponent


[Run]
;Filename: {sys}\regsvr32.exe; Parameters: "/s ""{app}""\OdbcFb.dll"; Components: DeveloperComponent DeploymentComponent


[UninstallRun]
;Filename: {sys}\regsvr32.exe; Parameters: "/u /s ""{app}""\OdbcFb.dll"; Components: DeveloperComponent DeploymentComponent


[UninstallDelete]
Type: Files; Name: {sys}\OdbcFb.dll; Components: DeveloperComponent DeploymentComponent


[CustomMessages]
en.DeveloperInstall=Developer install - register driver in System Dir. Install documentation to program group.
en.DeploymentInstall=Deployment install - no docs, no menus, no icons.
en.DocumentationInstall=Install documentation only.
en.DeveloperComponent=Install driver to
en.DeploymentComponent=Install driver only. No docs, uninstall.
en.DocumentationComponent=Documentation in CHM and HTML format

ru.DeveloperInstall=Полная установка - драйвер в системную папку, документацию в группу программ.
ru.DeploymentInstall=Только драйвер - никаких документов, меню, иконок.
ru.DocumentationInstall=Только документацию.
ru.DeveloperComponent=Установка драйвера в
ru.DeploymentComponent=Установка только драйвера, без документации и деинсталяции.
ru.DocumentationComponent=Документация в формате CHM и HTML


[Code]
procedure CurStepChanged(CurStep: TSetupStep);
var
  astring: string;
begin
  case CurStep of
    ssDone: begin
      astring := WizardSetupType(false)
      //Force deletion of install directory IF we do a deployment install
      if LowerCase(astring) = LowerCase('DeploymentInstall') then
        DelTree(ExpandConstant('{app}'), true,true,true)
    end;
  end;
end;
