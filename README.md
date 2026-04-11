# firebird-odbc-driver

Firebird ODBC driver for Firebird 3.0 and later.

[![Build and Test](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/FirebirdSQL/firebird-odbc-driver/actions/workflows/build-and-test.yml)

## Downloads

Download the latest release from the [GitHub Releases](https://github.com/FirebirdSQL/firebird-odbc-driver/releases) page.

| Platform | Files |
|----------|-------|
| Windows x64 | `.msi` installer, `.zip` archive |
| Linux x64 | `.tar.gz` archive |



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

### Prerequisites

Install [PowerShell](https://github.com/PowerShell/PowerShell) (v7+), then run:

```powershell
./install-prerequisites.ps1
```

This installs the required PowerShell modules ([InvokeBuild](https://github.com/nightroman/Invoke-Build) and [PSFirebird](https://github.com/fdcastel/PSFirebird)).

On Linux, you also need the unixODBC development package:

```bash
sudo apt-get install unixodbc unixodbc-dev
```

### Building

```powershell
Invoke-Build build -Configuration Release
```

This runs CMake to configure and build the driver. The output is:
- **Windows**: `build/Release/FirebirdODBC.dll`
- **Linux**: `build/libOdbcFb.so`

### Testing

```powershell
Invoke-Build test -Configuration Release
```

This will build the driver, download Firebird 5.0, create test databases, register the ODBC driver, and run the full test suite with multiple charset configurations.

### Available tasks

| Task | Description |
|------|-------------|
| `build` | Build the driver and tests (default) |
| `test` | Build, install driver, create test databases, run tests |
| `install` | Register the ODBC driver on the system |
| `uninstall` | Unregister the ODBC driver |
| `clean` | Remove the build directory |

### Alternative: Visual Studio

After running `cmake -B build`, you can open `build/firebird-odbc-driver.sln` with Visual Studio 2022 or later.

## Development & Feedback

- https://github.com/FirebirdSQL/firebird-odbc-driver

