<#
.Synopsis
    Install prerequisites for building and testing the Firebird ODBC Driver.

.Description
    Installs required PowerShell modules: PSFirebird and InvokeBuild.
    This script must be separate from the Invoke-Build script because
    it installs Invoke-Build itself.
#>

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

# Remove any cmd.exe AutoRun that can corrupt exit codes when Ninja spawns cmd /c.
# Some Windows Server images ship with an AutoRun (e.g. OPENFILES) that leaves
# ERRORLEVEL=1, breaking CMake/Ninja builds.
if ($IsWindows -or (-not $IsLinux -and -not $IsMacOS)) {
    Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Command Processor' -Name 'AutoRun' -ErrorAction SilentlyContinue
    Remove-ItemProperty -Path 'HKLM:\Software\Microsoft\Command Processor' -Name 'AutoRun' -ErrorAction SilentlyContinue
}

Set-PSRepository -Name PSGallery -InstallationPolicy Trusted

Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force -Scope CurrentUser -ForceBootstrap -ErrorAction Continue

if ($IsLinux) {
    Register-PackageSource -Name 'NuGet' -Location 'https://api.nuget.org/v3/index.json' -ProviderName NuGet -Force
}

Install-Module -Name PSFirebird -Force -AllowClobber -Scope CurrentUser -Repository PSGallery
Install-Module -Name InvokeBuild -Force -Scope CurrentUser
