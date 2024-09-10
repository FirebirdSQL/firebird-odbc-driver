# firebird-odbc-driver

Firebird ODBC driver v3.0

Welcome to the latest release of the Firebird ODBC driver v3.0. This release
sees many significant advances in the driver.
The most notable is that this version has OOAPI implementation inside.

This version is for Firebird 3.0 and later clients only.

All the new features and fixes are documented here - 
* [Release Notes](https://html-preview.github.io/?url=https://github.com/FirebirdSQL/firebird-odbc-driver/blob/feature_ooapi/Install/ReleaseNotes_v3.0.html)
* [ChangeLog](https://raw.githubusercontent.com/FirebirdSQL/firebird-odbc-driver/feature_ooapi/ChangeLog_v3.0)

## Downloads
The latest build artifacts:
* [Windows installation package](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/runs/10705194509/artifacts/1891984939) [![MSBuild](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/msbuild.yml/badge.svg)](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/msbuild.yml)
* [Linux so binary](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/runs/10705194518/artifacts/1891959862) [![Linux](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/linux.yml/badge.svg)](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/linux.yml)

You can also download the lastest & archive build packages here: https://github.com/FirebirdSQL/firebird-odbc-driver/wiki


## Build from sources

### Linux
* Clone the git repository into your working copy folder
* Make sure you have Unix ODBC dev package installed. If not - install it (for example: `sudo apt install unixodbc-dev` for Ubuntu)
* Move to Builds/Gcc.lin
* Rename makefile.linux -> makefile
* Set the DEBUG var if you need a Debug build instead of Release (by default)
* Run `make`
* Your libraries are in ./Release_<arch> or ./Debug_<arch> folder.

### Windows
* Clone the git repository into your working copy folder
* Open `<working copy folder>`/Builds/MsVc2022.win/OdbcFb.sln with MS Visual Studio (VS2022 or later)
* Select your desired arch & build mode (debug|release)
* Build the project
* Copy the built library (`<working copy folder>`\Builds\MsVc2022.win\\`arch`\\`build_mode`\FirebirdODBC.dll) to `<Windows>`\System32 (x64 arch) or `<Windows>`\SysWOW64 (Win32 arch)

## Development & Feedback

- https://github.com/FirebirdSQL/firebird-odbc-driver

