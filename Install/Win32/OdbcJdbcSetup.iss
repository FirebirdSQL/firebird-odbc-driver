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
; for MsVC 6.0
#define sourceDll="..\..\Builds\MsVc60.win\Release\"

[Setup]
AppName=Firebird ODBC
AppVerName=OdbcJdbc version 1-1-beta
WizardImageFile=firebird-logo1.bmp
WizardImageBackColor=clWhite
WizardSmallImageFile=firebird-logo2.bmp
WizardSmallImageBackColor=clWhite
DefaultDirName={pf}\Firebird ODBC
DefaultGroupName=Firebird ODBC
UninstallDisplayIcon={app}\OdbcJdbcSetup.dll

[Files]

Source: "{#sourceDll}IscDbc.dll"; DestDir: "{app}"; Flags: ignoreversion deleteafterinstall
Source: "{#sourceDll}OdbcJdbc.dll"; DestDir: "{app}"; Flags: ignoreversion deleteafterinstall
Source: "{#sourceDll}OdbcJdbcSetup.dll"; DestDir: "{app}"; Flags: ignoreversion
;Source: "FirebirdOdbc.hlp"; DestDir: "{app}"
Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
Name: "{group}\Firebird ODBC"; Filename: "{uninstallexe}"

[Run]
Filename: "{sys}\regsvr32.exe"; Parameters: "/s ""{app}""\OdbcJdbcSetup.dll"

[UninstallRun]
Filename: "{sys}\regsvr32.exe"; Parameters: "/u/s ""{app}""\OdbcJdbcSetup.dll"

