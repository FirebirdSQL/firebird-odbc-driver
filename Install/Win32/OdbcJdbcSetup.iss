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
;  Currently compiled against InnoSetup v5.3 from http://www.innosetup.com/
;
;

#define OBJNAME "FirebirdODBC"

#define MSVC_VERSION 11
#define BUILDCONFIG "release"

#if MSVC_VERSION==7
#define BUILD_ENV "MsVc70.win"
#elif MSVC_VERSION==8
#define BUILD_ENV "MsVc80.win"
#elif MSVC_VERSION==9
#define BUILD_ENV "MsVc90.win"
#elif MSVC_VERSION==11
#define BUILD_ENV "MsVc2022.win"
#else
BUILD_ENV undefined
#endif

#if BUILDCONFIG=="debug"
#define debug_str "_debug"
#else
#define debug_str ""
#endif

#define FIREBIRD_URL "http://www.firebirdsql.org"

;---- If we haven't already set PlatformTarget then pick it up from the environment.
#ifndef PlatformTarget
#define PlatformTarget GetEnv("FB_TARGET_PLATFORM")
#endif
#if PlatformTarget == ""
#define PlatformTarget "win32"
#endif

;---- If we haven't already set ProductVersion then pick it up from the environment.
#ifndef ProductVersion
#define ProductVersion GetEnv("PRODUCT_VERSION")
#endif

#define BUILD_ROOT="..\..\"
#define SOURCE_LIBS "Builds\"+AddBackslash(BUILD_ENV)+AddBackslash(PlatformTarget)+AddBackslash(BUILDCONFIG)
#define SOURCE_DOCS="Install\"

#if PlatformTarget == "x64"
#define SOURCE_LIBS32="Builds\"+AddBackslash(BUILD_ENV)+AddBackslash("Win32")+AddBackslash(BUILDCONFIG)
#endif

; Check if HTML help is available
#ifndef HtmlHelp
#define HtmlHelp GetEnv("HTMLHELP")
#endif
#if HtmlHelp == ""
#undef HtmlHelp
#endif

[Setup]
DisableDirPage=No
AppName=Firebird ODBC Driver 3
AppVerName=Firebird ODBC driver {#ProductVersion}
AppVersion={#ProductVersion}
AppMutex=InnoSetupExtensionsCompilerAppMutex
AppPublisher=Firebird Project
AppPublisherURL={#FIREBIRD_URL}
AppSupportURL={#FIREBIRD_URL}
AppUpdatesURL={#FIREBIRD_URL}

DefaultDirName={pf}\Firebird\Firebird_ODBC_3
DefaultGroupName=Firebird\Firebird ODBC Driver 3
UninstallDisplayIcon={sys}\{#OBJNAME}.dll
UninstallFilesDir={localappdata}\{#OBJNAME}

PrivilegesRequired=admin

SourceDir={#BUILD_ROOT}
OutputDir={#SOURCE_DOCS}\Win32\install_image
OutputBaseFilename=Firebird_ODBC_{#ProductVersion}_{#PlatformTarget}{#debug_str}
DiskSpanning=no

LicenseFile={#SOURCE_DOCS}\IDPLicense.txt
InfoBeforeFile={#SOURCE_DOCS}\Win32\installation_readme.txt
InfoAfterFile={#SOURCE_DOCS}\Win32\readme.txt
Compression=lzma
SolidCompression=true

WizardImageFile={#SOURCE_DOCS}\Win32\firebird-logo1.bmp
WizardImageBackColor=clWhite
WizardSmallImageFile={#SOURCE_DOCS}\Win32\firebird-logo2.bmp

#if PlatformTarget == "x64"
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
#endif

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
Source: {#SOURCE_LIBS}{#OBJNAME}.dll; DestDir: {sys}; Components: DeveloperComponent DeploymentComponent; Flags: regserver restartreplace sharedfile
Source: {#SOURCE_LIBS}\{#OBJNAME}.lib; DestDir: {sys}; Components: DeveloperComponent DeploymentComponent
Source: {#SOURCE_LIBS}\{#OBJNAME}.pdb; DestDir: {sys}; Components: DeveloperComponent DeploymentComponent
#ifdef HtmlHelp
Source: {#SOURCE_DOCS}\HtmlHelp\{#OBJNAME}.chm; DestDir: {app}; Components: DeveloperComponent DeploymentComponent
Source: {#SOURCE_DOCS}\HtmlHelp\{#OBJNAME}.chm; DestDir: {sys}; Components: DeveloperComponent DeploymentComponent
Source: {#SOURCE_DOCS}\HtmlHelp\{#OBJNAME}.chm; DestDir: {app}; Components: DocumentationComponent
Source: {#SOURCE_DOCS}\HtmlHelp\{#OBJNAME}.chm; DestDir: {sys}; Components: DocumentationComponent
Source: {#SOURCE_DOCS}\HtmlHelp\html\*.*; DestDir: {app}\html; Components: DocumentationComponent
Source: {#SOURCE_DOCS}\HtmlHelp\images\*.*; DestDir: {app}\images; Components: DocumentationComponent
#endif
Source: {#SOURCE_DOCS}\Win32\Readme.txt; DestDir: {app}; Components: DocumentationComponent; Flags: isreadme
Source: {#SOURCE_DOCS}\IDPLicense.txt; DestDir: {app}; Components: DocumentationComponent
;Source: {#SOURCE_DOCS}\ReleaseNotes_v2.0.html; DestDir: {app}; Components: DocumentationComponent

#if PlatformTarget == "x64"
Source: {#SOURCE_LIBS32}{#OBJNAME}.dll; DestDir: {sys}; Components: DeveloperComponent DeploymentComponent; Flags: regserver restartreplace 32bit
Source: {#SOURCE_LIBS32}\{#OBJNAME}.lib; DestDir: {syswow64}; Components: DeveloperComponent DeploymentComponent
Source: {#SOURCE_LIBS32}\{#OBJNAME}.pdb; DestDir: {syswow64}; Components: DeveloperComponent DeploymentComponent
#ifdef HtmlHelp
Source: {#SOURCE_DOCS}\HtmlHelp\{#OBJNAME}.chm; DestDir: {syswow64}; Components: DeveloperComponent DeploymentComponent
Source: {#SOURCE_DOCS}\HtmlHelp\{#OBJNAME}.chm; DestDir: {syswow64}; Components: DocumentationComponent
#endif
#endif

[Icons]
Name: {group}\Uninstall Firebird ODBC driver; Filename: {uninstallexe}; Components: DocumentationComponent; Comment: Remove Firebird ODBC Driver Documentation
Name: {group}\Uninstall Firebird ODBC driver; Filename: {uninstallexe}; Components: DeveloperComponent; Comment: Remove Firebird ODBC Driver Library and Documentation
#ifdef HtmlHelp
Name: {group}\Firebird ODBC Help; Filename: {app}\{#OBJNAME}.chm; Components: DocumentationComponent
Name: {group}\Firebird ODBC Help; Filename: {sys}\{#OBJNAME}.chm; Components: DeveloperComponent
Name: {app}\Firebird ODBC Help; Filename: {sys}\{#OBJNAME}.chm; Components: DeveloperComponent
#endif
;Name: {group}\Firebird ODBC v2.0 Release Notes; Filename: {app}\ReleaseNotes_v2.0.html; Components: DocumentationComponent
Name: {group}\Firebird ODBC readme.txt; Filename: {app}\Readme.txt; Components: DocumentationComponent
Name: {group}\Firebird ODBC license.txt; Filename: {app}\IDPLicense.txt; Components: DocumentationComponent


[Run]
;Filename: {sys}\regsvr32.exe; Parameters: "/s ""{app}""\{#OBJNAME}.dll"; Components: DeveloperComponent DeploymentComponent


[UninstallRun]
;Filename: {sys}\regsvr32.exe; Parameters: "/u /s ""{app}""\{#OBJNAME}.dll"; Components: DeveloperComponent DeploymentComponent


[UninstallDelete]
Type: Files; Name: {sys}\{#OBJNAME}.dll; Components: DeveloperComponent DeploymentComponent


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
