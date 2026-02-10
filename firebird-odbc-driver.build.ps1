<#
.Synopsis
	Build script for Firebird ODBC Driver (Invoke-Build)

.Description
	Tasks: clean, build, test, install, uninstall.

.Parameter Configuration
	Build configuration: Debug (default) or Release.
#>

param(
	[ValidateSet('Debug', 'Release')]
	[string]$Configuration = 'Debug'
)

# Detect OS
$IsWindowsOS = $IsWindows -or ($PSVersionTable.PSVersion.Major -le 5)
$IsLinuxOS = $IsLinux

# Computed properties
$BuildDir = Join-Path $BuildRoot 'build'

$DriverName = if ($Configuration -eq 'Release') {
	'Firebird ODBC Driver'
} else {
	'Firebird ODBC Driver (Debug)'
}

if ($IsWindowsOS) {
	$DriverFileName = 'FirebirdODBC.dll'
	$DriverPath = Join-Path $BuildDir $Configuration $DriverFileName
} else {
	$DriverFileName = 'libOdbcFb.so'
	$DriverPath = Join-Path $BuildDir $DriverFileName
}

# Synopsis: Remove the build directory.
task clean {
	remove $BuildDir
}

# Synopsis: Build the driver and tests (default task).
task build {
	exec { cmake -B $BuildDir -S $BuildRoot -DCMAKE_BUILD_TYPE=$Configuration -DBUILD_TESTING=ON }

	if ($IsWindowsOS) {
		exec { cmake --build $BuildDir --config $Configuration }
	} else {
		$jobs = 4
		try { $jobs = [int](nproc) } catch {}
		exec { cmake --build $BuildDir --config $Configuration -- "-j$jobs" }
	}

	assert (Test-Path $DriverPath) "Driver not found at: $DriverPath"
}

# Synopsis: Create Firebird test databases.
task build-test-databases {
	$fbVersion = '5.0.2'
	$envPath = '/fbodbc-tests/fb502'
	$dbPathUtf8 = '/fbodbc-tests/TEST.FB50.FDB'
	$dbPathIso = '/fbodbc-tests/TEST-ISO.FB50.FDB'

	if ($IsLinuxOS) {
		exec { sudo mkdir -p '/fbodbc-tests' }
		exec { sudo chmod 777 '/fbodbc-tests' }
	} else {
		New-Item -ItemType Directory -Path '/fbodbc-tests' -Force | Out-Null
	}

	Import-Module PSFirebird

	# Create or reuse Firebird environment
	if (Test-Path (Join-Path $envPath 'firebird.msg')) {
		$fb = Get-FirebirdEnvironment -Path $envPath
		print Green "Reusing existing Firebird environment: $envPath"
	} else {
		$fb = New-FirebirdEnvironment -Version $fbVersion -Path $envPath -Force
		print Green "Firebird environment created: $envPath"
	}

	# Stop any running Firebird processes so database files can be replaced
	if ($IsWindowsOS) {
		Get-Service -Name 'FirebirdServer*' -ErrorAction SilentlyContinue |
			Where-Object Status -eq 'Running' |
			Stop-Service -Force -ErrorAction SilentlyContinue
	}
	$fbProcs = Get-Process -Name 'firebird' -ErrorAction SilentlyContinue
	if ($fbProcs) {
		$fbProcs | Stop-Process -Force -ErrorAction SilentlyContinue
		$fbProcs | Wait-Process -Timeout 10 -ErrorAction SilentlyContinue
	}
	if ($IsLinuxOS) {
		sudo pkill -f firebird 2>$null
		Start-Sleep -Seconds 2
	}

	New-FirebirdDatabase -Database $dbPathUtf8 -Environment $fb -Force
	print Green "UTF8 database created: $dbPathUtf8"

	New-FirebirdDatabase -Database $dbPathIso -Environment $fb -Charset ISO8859_1 -Force
	print Green "ISO8859_1 database created: $dbPathIso"

	# Determine client library path
	if ($IsWindowsOS) {
		$script:ClientPath = (Resolve-Path (Join-Path $fb.Path 'fbclient.dll')).Path
	} else {
		$clientLib = Join-Path $fb.Path 'lib' 'libfbclient.so'
		if (-not (Test-Path $clientLib)) {
			$clientLib = Get-ChildItem -Path $fb.Path -Recurse -Filter 'libfbclient.so*' |
				Select-Object -First 1 -ExpandProperty FullName
		}
		$script:ClientPath = $clientLib
	}

	print Green "Client library: $script:ClientPath"
}

# Synopsis: Run the test suite.
task test build, build-test-databases, install, {
	if (-not $env:FIREBIRD_ODBC_CONNECTION) {
		print Yellow 'WARNING: FIREBIRD_ODBC_CONNECTION environment variable is not set. Using built-in connection strings.'
	}

	# Test suites that exercise charset/encoding-sensitive code paths.
	# These are the only suites re-run with non-default charset configurations
	# because their behavior changes based on connection charset or database charset.
	# All other suites use pure ASCII text with SQL_C_CHAR bindings, so charset
	# configuration doesn't affect their results.
	$charsetSensitiveSuites = @(
		'WCharTest'              # W-API / SQLWCHAR paths, UTF-8↔UTF-16 conversions
		'DataTypeTest'           # VARCHAR/CHAR type reporting, text round-trips
		'CatalogTest'            # Metadata strings, type names
		'CatalogFunctionsTest'   # Extensive metadata queries
		'DescRecTest'            # SQL type reporting (VARCHAR vs WVARCHAR)
		'TypeInfoTest'           # SQLGetTypeInfo strings
		'BlobTest'               # Text blob encoding
		'ResultConversionsTest'  # Text conversion paths (int→string, date→string)
		'ParamConversionsTest'   # Parameter text paths
		'EscapeSequenceTest'     # String function results
	)
	$charsetFilter = ($charsetSensitiveSuites | ForEach-Object { "$_.*" }) -join ':'

	$testConfigs = @(
		@{ Database = '/fbodbc-tests/TEST.FB50.FDB';     Charset = 'UTF8';      Label = 'UTF8 database, UTF8 charset';      Filter = $null }
		@{ Database = '/fbodbc-tests/TEST-ISO.FB50.FDB'; Charset = 'ISO8859_1'; Label = 'ISO8859_1 database, ISO8859_1 charset'; Filter = $charsetFilter }
		@{ Database = '/fbodbc-tests/TEST-ISO.FB50.FDB'; Charset = 'UTF8';      Label = 'ISO8859_1 database, UTF8 charset';     Filter = $charsetFilter }
	)

	$passed = 0
	foreach ($cfg in $testConfigs) {
		$env:FIREBIRD_ODBC_CONNECTION = "Driver={$DriverName};Database=$($cfg.Database);UID=SYSDBA;PWD=masterkey;CHARSET=$($cfg.Charset);CLIENT=$script:ClientPath"
		print Cyan "--- Test run: $($cfg.Label) ---"
		print Cyan "    Connection: $env:FIREBIRD_ODBC_CONNECTION"

		if ($cfg.Filter) {
			# Run only charset-sensitive suites for non-default configurations
			$testExe = if ($IsWindowsOS) {
				Join-Path $BuildDir 'tests' $Configuration 'firebird_odbc_tests.exe'
			} else {
				Join-Path $BuildDir 'tests' 'firebird_odbc_tests'
			}
			print Cyan "    Filter: $($cfg.Filter)"
			exec { & $testExe --gtest_filter=$($cfg.Filter) }
		} else {
			# Run full suite for the primary (UTF8) configuration
			exec { ctest --test-dir $BuildDir -C $Configuration --output-on-failure }
		}
		$passed++
	}

	print Green "All $passed test runs passed."
}

# Synopsis: Register the ODBC driver on the system.
task install build, {
	if ($IsWindowsOS) {
		Install-WindowsDriver
	} elseif ($IsLinuxOS) {
		Install-LinuxDriver
	} else {
		throw 'Unsupported OS. Only Windows and Linux are supported.'
	}
}

# Synopsis: Unregister the ODBC driver from the system.
task uninstall {
	if ($IsWindowsOS) {
		Uninstall-WindowsDriver
	} elseif ($IsLinuxOS) {
		Uninstall-LinuxDriver
	} else {
		throw 'Unsupported OS. Only Windows and Linux are supported.'
	}
}

# Synopsis: Build the driver and tests.
task . build

# Synopsis: Run performance benchmarks.
task benchmark build, build-test-databases, install, {
	if (-not $env:FIREBIRD_ODBC_CONNECTION) {
		print Yellow 'WARNING: FIREBIRD_ODBC_CONNECTION environment variable is not set. Using built-in connection strings.'
	}

	# Use the UTF8 database for benchmarks
	$env:FIREBIRD_ODBC_CONNECTION = "Driver={$DriverName};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=$script:ClientPath"
	print Cyan "--- Running benchmarks ---"
	print Cyan "    Connection: $env:FIREBIRD_ODBC_CONNECTION"

	$benchExe = if ($IsWindowsOS) {
		Join-Path $BuildDir "tests" $Configuration "firebird_odbc_bench.exe"
	} else {
		Join-Path $BuildDir "tests" "firebird_odbc_bench"
	}

	assert (Test-Path $benchExe) "Benchmark executable not found at: $benchExe"

	$jsonOutput = Join-Path $BuildRoot 'tmp' 'benchmark_results.json'
	New-Item -ItemType Directory -Path (Split-Path $jsonOutput) -Force | Out-Null

	exec { & $benchExe --benchmark_format=console --benchmark_out=$jsonOutput --benchmark_out_format=json --benchmark_repetitions=3 --benchmark_min_time=2s }

	print Green "Benchmark results saved to: $jsonOutput"
}

#region Windows

function Install-WindowsDriver {
	$regBase = 'HKLM:\SOFTWARE\ODBC\ODBCINST.INI'
	$regPath = Join-Path $regBase $DriverName
	$driversPath = Join-Path $regBase 'ODBC Drivers'

	if (!(Test-Path $regPath)) {
		New-Item -Path $regBase -Name $DriverName -Force | Out-Null
	}

	Set-ItemProperty -Path $regPath -Name 'Driver' -Value $DriverPath -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'Setup' -Value $DriverPath -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'APILevel' -Value '1' -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'ConnectFunctions' -Value 'YYY' -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'DriverODBCVer' -Value '03.51' -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'FileUsage' -Value '0' -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'SQLLevel' -Value '1' -Type String -Force

	if (!(Test-Path $driversPath)) {
		New-Item -Path $regBase -Name 'ODBC Drivers' -Force | Out-Null
	}
	Set-ItemProperty -Path $driversPath -Name $DriverName -Value 'Installed' -Type String -Force

	print Green "Driver '$DriverName' registered: $DriverPath"
}

function Uninstall-WindowsDriver {
	$regBase = 'HKLM:\SOFTWARE\ODBC\ODBCINST.INI'
	$regPath = Join-Path $regBase $DriverName
	$driversPath = Join-Path $regBase 'ODBC Drivers'

	if (Test-Path $regPath) {
		Remove-Item -Path $regPath -Recurse -Force
	}
	if (Test-Path $driversPath) {
		Remove-ItemProperty -Path $driversPath -Name $DriverName -ErrorAction SilentlyContinue
	}

	print Green "Driver '$DriverName' unregistered."
}

#endregion

#region Linux

function Install-LinuxDriver {
	if (-not (Get-Command odbcinst -ErrorAction Ignore)) {
		throw 'odbcinst not found. Install unixODBC: sudo apt-get install unixodbc'
	}

	$driverAbsPath = (Resolve-Path $DriverPath).Path

	$tempIni = [System.IO.Path]::GetTempFileName()
	try {
		@"
[$DriverName]
Description = $DriverName
Driver = $driverAbsPath
Setup = $driverAbsPath
Threading = 0
FileUsage = 0
"@ | Set-Content -Path $tempIni

		exec { sudo odbcinst -i -d -f $tempIni -r }
	} finally {
		Remove-Item -Path $tempIni -Force -ErrorAction SilentlyContinue
	}

	print Green "Driver '$DriverName' registered: $driverAbsPath"
}

function Uninstall-LinuxDriver {
	if (-not (Get-Command odbcinst -ErrorAction Ignore)) {
		throw 'odbcinst not found. Install unixODBC: sudo apt-get install unixodbc'
	}

	exec { odbcinst -u -d -n $DriverName }

	print Green "Driver '$DriverName' unregistered."
}

#endregion
