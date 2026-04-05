# Firebird ODBC Driver

An ODBC driver for [Firebird](https://firebirdsql.org/) databases. Supports Firebird 3.0, 4.0, and 5.0 on Windows and Linux (x64, ARM64).

## Features

- Full ODBC 3.51 API compliance
- Unicode support (ANSI and Wide-character ODBC entry points)
- Connection string and DSN-based connections
- Prepared statements with parameter binding
- Stored procedure execution (EXECUTE PROCEDURE)
- Catalog functions (SQLTables, SQLColumns, SQLPrimaryKeys, etc.)
- Transaction control (manual and auto-commit modes)
- Statement-level savepoint isolation (failed statements don't corrupt the transaction)
- BLOB read/write support
- Scrollable cursors (static)
- Thread-safe (connection-level locking)

## Quick Start

### Connection String

Connect using `SQLDriverConnect` with a connection string:

```
Driver={Firebird ODBC Driver};Database=localhost:C:\mydb\employee.fdb;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8
```

### Connection Parameters

| Parameter | Alias | Description | Default |
|-----------|-------|-------------|---------|
| `Driver` | — | ODBC driver name (must be `{Firebird ODBC Driver}`) | — |
| `DSN` | — | Data Source Name (alternative to Driver) | — |
| `Database` | `Dbname` | Database path (`host:path` or just `path` for local) | — |
| `UID` | `User` | Username | `SYSDBA` |
| `PWD` | `Password` | Password | — |
| `Role` | — | SQL role for the connection | — |
| `CHARSET` | `CharacterSet` | Character set for the connection (see below) | `UTF8` |
| `Client` | — | Path to `fbclient.dll` / `libfbclient.so` | System default |
| `Dialect` | — | SQL dialect (1 or 3) | `3` |
| `ReadOnly` | — | Read-only transaction mode (`Y`/`N`) | `N` |
| `NoWait` | — | No-wait transaction mode (`Y`/`N`) | `N` |
| `LockTimeout` | — | Lock timeout in seconds for WAIT transactions | `0` (infinite) |
| `Quoted` | `QuotedIdentifier` | Enable quoted identifiers (`Y`/`N`) | `Y` |
| `Sensitive` | `SensitiveIdentifier` | Case-sensitive identifiers (`Y`/`N`) | `N` |
| `AutoQuoted` | `AutoQuotedIdentifier` | Automatically quote identifiers (`Y`/`N`) | `N` |
| `UseSchema` | `UseSchemaIdentifier` | Schema handling (0=none, 1=remove, 2=full) | `0` |
| `SafeThread` | — | Thread safety mode | `Y` |
| `PageSize` | — | Default page size for CREATE DATABASE | `4096` |
| `EnableWireCompression` | — | Enable wire compression (`Y`/`N`) | `N` |

### Connection String Examples

```
# Remote server with explicit client library
Driver={Firebird ODBC Driver};Database=myserver:C:\databases\mydb.fdb;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=C:\Firebird\fbclient.dll

# Local database on Linux
Driver={Firebird ODBC Driver};Database=/var/db/mydb.fdb;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8

# Read-only connection with lock timeout
Driver={Firebird ODBC Driver};Database=myserver:/data/mydb.fdb;UID=SYSDBA;PWD=masterkey;ReadOnly=Y;LockTimeout=10
```

### Character Set (`CHARSET`)

The `CHARSET` parameter controls what character encoding Firebird uses when sending text data over the wire. It defaults to **`UTF8`** when not specified.

| CHARSET Value | Behavior | Recommended? |
|---|---|---|
| `UTF8` (default) | Firebird transliterates all text to UTF-8. The driver converts UTF-8 → UTF-16 for W-API calls. Full Unicode support. | ✅ **Yes** — use this for all new applications |
| `ISO8859_1`, `WIN1252`, etc. | Firebird transliterates to the specified charset. The driver converts to UTF-16 via the charset's codec. Only characters in that charset's repertoire are representable. | ⚠️ Legacy — use only if you know your data fits this charset |
| `NONE` | Firebird sends raw bytes in the column's storage charset without transliteration. On Windows, the driver uses the system ANSI code page for W-API conversion. On Linux, the driver falls back to UTF-8 decoding. | ❌ **Avoid** — can produce mojibake with multi-charset databases |

> **Note**: When `CHARSET` is omitted, the driver defaults to `UTF8` and emits an informational diagnostic (SQLSTATE 01000) so applications are aware a default was applied.

---

## Building from Source

### Prerequisites

| Requirement | Version |
|-------------|---------|
| C++ compiler | C++17 capable (MSVC 2019+, GCC 7+, Clang 5+) |
| CMake | 3.20 or later |
| PowerShell | 7.0 or later |
| [vcpkg](https://github.com/microsoft/vcpkg) | Latest |
| [Invoke-Build](https://github.com/nightroman/Invoke-Build) | Any |
| ODBC headers | Windows SDK (included) or unixODBC-dev (Linux) |

### vcpkg Setup

All C++ dependencies (Firebird client headers, Google Test, Google Benchmark) are managed by [vcpkg](https://github.com/microsoft/vcpkg) via the manifest files `vcpkg.json` and `vcpkg-configuration.json`.

**Install vcpkg** (if not already installed):

```bash
# Clone vcpkg (one-time)
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && bootstrap-vcpkg.bat   # Windows
cd vcpkg && ./bootstrap-vcpkg.sh  # Linux/macOS
```

**Set the `VCPKG_ROOT` environment variable** to point to your vcpkg installation:

```powershell
# PowerShell (Windows)
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# Bash (Linux/macOS)
export VCPKG_ROOT=/path/to/vcpkg
```

CMake automatically picks up the vcpkg toolchain from `VCPKG_ROOT`. Dependencies are installed automatically during the first build via vcpkg manifest mode — no manual `vcpkg install` is needed.

> **Note**: The first build takes longer (~10–15 minutes on Windows) as vcpkg downloads and builds dependencies. Subsequent builds use the binary cache and are much faster.

Install Invoke-Build (one-time):

```powershell
Install-Module InvokeBuild -Scope CurrentUser
```

### Build

```powershell
# Debug build (default)
Invoke-Build build

# Release build
Invoke-Build build -Configuration Release
```

The driver output is at `build\<Configuration>\FirebirdODBC.dll` (Windows) or `build/libOdbcFb.so` (Linux).

### Install / Uninstall

Register the built driver in the system ODBC driver list:

```powershell
# Install — Debug configuration registers as "Firebird ODBC Driver (Debug)"
Invoke-Build install

# Install — Release configuration registers as "Firebird ODBC Driver"
Invoke-Build install -Configuration Release

# Uninstall
Invoke-Build uninstall
Invoke-Build uninstall -Configuration Release
```

On Windows this writes to the registry (`HKLM:\SOFTWARE\ODBC\ODBCINST.INI`). On Linux this uses `odbcinst`.

### Clean

```powershell
Invoke-Build clean
```

---

## Running the Tests

Tests use [Google Test](https://github.com/google/googletest/) and are managed via vcpkg.

### Without a database (unit tests only)

```powershell
Invoke-Build test
```

### With a Firebird database (full integration tests)

Set the `FIREBIRD_ODBC_CONNECTION` environment variable to enable integration tests. Tests that require a database connection will be skipped (not failed) when this variable is not set.

```powershell
# PowerShell (Windows)
$env:FIREBIRD_ODBC_CONNECTION='Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=C:\Firebird\fbclient.dll'
Invoke-Build test
```

```powershell
# PowerShell (Linux)
$env:FIREBIRD_ODBC_CONNECTION='Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=/usr/lib/libfbclient.so'
Invoke-Build test
```

### CI

The project includes GitHub Actions workflows (`.github/workflows/build-and-test.yml`) that automatically:

- Build on Windows x64 and Linux x64
- Set up a Firebird 5.0 test database using [PSFirebird](https://github.com/fdcastel/PSFirebird)
- Register the ODBC driver
- Run all tests (unit + integration)

---

## Project Structure

```
├── firebird-odbc-driver.build.ps1  # Invoke-Build script (build, test, install, uninstall)
├── CMakeLists.txt          # Top-level CMake build configuration
├── Main.cpp                # ODBC ANSI entry points (SQLConnect, SQLExecDirect, etc.)
├── MainUnicode.cpp         # ODBC Unicode (W) entry points (SQLConnectW, etc.)
├── OdbcConnection.cpp/h    # ODBC connection handle implementation
├── OdbcStatement.cpp/h     # ODBC statement handle implementation
├── OdbcEnv.cpp/h           # ODBC environment handle implementation
├── OdbcDesc.cpp/h          # ODBC descriptor handle implementation
├── OdbcConvert.cpp/h       # Data type conversions between C and SQL types
├── OdbcError.cpp/h         # Diagnostic record management
├── OdbcObject.cpp/h        # Base class for all ODBC handle objects
├── SafeEnvThread.cpp/h     # Thread-safety (connection-level mutexes)
├── IscDbc/                 # Firebird client interface layer (IscConnection, IscStatement, etc.)
├── FBClient.Headers/       # Firebird client API headers (no Firebird install needed to build)
├── Headers/                # ODBC API headers for cross-platform builds
├── tests/                  # Google Test-based test suite
├── Docs/                   # Documentation and project roadmap
├── .github/workflows/      # CI/CD pipelines
└── Builds/                 # Legacy build files (Makefiles for various compilers)
```

---

## License

The source code is released under variants of the Mozilla Public Licence 1.1 (MPL):

- https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
- https://www.firebirdsql.org/en/interbase-public-license/
