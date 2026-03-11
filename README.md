# firebird-odbc-driver

Firebird ODBC driver v3.0

Welcome to the latest release of the Firebird ODBC driver v3.0. This release
sees many significant advances in the driver.
The most notable is that this version has OOAPI implementation inside.

This version is for Firebird 3.0 and later clients only.

All the new features and fixes are documented here - 
* [Release Notes](https://html-preview.github.io/?url=https://github.com/FirebirdSQL/firebird-odbc-driver/blob/master/Install/ReleaseNotes_v3.0.html)
* [ChangeLog](https://raw.githubusercontent.com/FirebirdSQL/firebird-odbc-driver/master/ChangeLog_v3.0)



## Downloads

The latest build artifacts:
* [Windows installation package](https://github.com/user-attachments/files/19207749/win_installers.zip) [![MSBuild](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/msbuild.yml/badge.svg)](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/msbuild.yml)
* [Linux x86-64](https://github.com/user-attachments/files/19207739/linux_libs.zip) [![Linux](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/linux.yml/badge.svg)](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/linux.yml)
* [Linux_ARM64](https://github.com/user-attachments/files/19210460/linux_arm64_libs.zip) [![RaspberryPI](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/rpi_arm64.yml/badge.svg)](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/rpi_arm64.yml)

You can also download the lastest & archive build packages here: https://github.com/FirebirdSQL/firebird-odbc-driver/wiki



## Connection String Parameters

### Required Parameters

| Parameter | Aliases | Description | Example |
|-----------|---------|-------------|---------|
| **DRIVER** | - | ODBC driver name | `Firebird ODBC Driver` |
| **DBNAME** | DATABASE | Database path or alias | `localhost:C:\data\mydb.fdb` |

### Authentication Parameters

| Parameter | Aliases | Description | Default |
|-----------|---------|-------------|---------|
| **UID** | USER | Database username | (none) |
| **PWD** | PASSWORD | Database password | (none) |
| **ROLE** | - | SQL role to assume | (none) |

### Optional Parameters

| Parameter | Aliases | Description | Values | Default |
|-----------|---------|-------------|--------|---------|
| **DSN** | - | Data source name | Any valid DSN | (none) |
| **CHARSET** | - | Character set for connection | `UTF8`, `WIN1252`, `ISO8859_1`, etc. | `NONE` |
| **CLIENT** | - | Path to Firebird client library | Full path to fbclient.dll/so | (auto-detect) |
| **DIALECT** | - | SQL dialect | `1` or `3` | `3` |
| **READONLY** | - | Read-only transaction mode | `Y` or `N` | `N` |
| **NOWAIT** | - | No-wait lock resolution | `Y` or `N` | `N` |
| **TIMEOUT** | - | Connection timeout (seconds) | Integer | `0` (disabled) |
| **QUOTED** | QUOTEDIDENTIFIER | Enable quoted identifiers | `Y` or `N` | `Y` |
| **SENSITIVE** | SENSITIVEIDENTIFIER | Case-sensitive identifiers | `Y` or `N` | `N` |
| **AUTOQUOTED** | AUTOQUOTEDIDENTIFIER | Auto-quote identifiers | `Y` or `N` | `N` |
| **USESCHEMA** | USESCHEMAIDENTIFIER | Schema handling mode | `0`, `1`, `2` | `0` |
| **LOCKTIMEOUT** | - | Lock timeout for transactions | Integer (seconds) | (none) |
| **SAFETHREAD** | - | Thread-safe operations | `Y` or `N` | `Y` |
| **PAGESIZE** | - | Database page size (creation only) | `4096`, `8192`, `16384` | `8192` |
| **ENABLECOMPATBIND** | - | Enable compatible bindings | `Y` or `N` | `Y` |
| **SETCOMPATBIND** | - | Set compatibility binding mode | Firebird compat string | (none) |
| **ENABLEWIRECOMPRESSION** | - | Enable wire protocol compression | `Y` or `N` | `N` |

### Parameter Details

#### DBNAME / DATABASE
Specifies the database location. Supports multiple formats:

- **Local**: `C:\data\mydb.fdb` or `/var/lib/firebird/mydb.fdb`
- **Remote**: `servername:/path/to/db.fdb` or `servername:C:\path\to\db.fdb`
- **TCP/IP with IP**: `192.168.1.100:/path/to/db.fdb`
- **Custom port**: `servername/3051:/path/to/db.fdb`
- **Alias**: `myalias` (defined in Firebird's `databases.conf`)

#### CHARSET
Sets the character encoding for the connection. Common values:
- `UTF8` - Unicode (recommended)
- `WIN1252` - Western European (Windows)
- `WIN1251` - Cyrillic
- `ISO8859_1` - Latin-1
- `NONE` - No character set translation

#### DIALECT
SQL dialect controls parsing behavior:
- `3` - Modern Firebird/InterBase 6+ mode (recommended)
- `1` - InterBase 5 compatibility mode (deprecated)

#### USESCHEMA
Controls schema name handling:
- `0` - Set NULL for SCHEMA field (default)
- `1` - Remove SCHEMA from SQL queries
- `2` - Use full SCHEMA support

#### QUOTED / SENSITIVE / AUTOQUOTED
Identifier handling options:
- **QUOTED=Y**: Allows quoted identifiers (e.g., `"MyTable"`)
- **SENSITIVE=Y**: Makes identifiers case-sensitive
- **AUTOQUOTED=Y**: Automatically quotes all identifiers

#### SAFETHREAD
- `Y` - Enable thread-safe connection handling (default, recommended)
- `N` - Disable thread safety (use only in single-threaded apps)



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

