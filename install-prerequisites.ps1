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

Set-PSRepository -Name PSGallery -InstallationPolicy Trusted

Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force -Scope CurrentUser -ForceBootstrap -ErrorAction Continue

if ($IsLinux) {
    Register-PackageSource -Name 'NuGet' -Location 'https://api.nuget.org/v3/index.json' -ProviderName NuGet -Force
}

Install-Module -Name PSFirebird -Force -AllowClobber -Scope CurrentUser -Repository PSGallery
Install-Module -Name InvokeBuild -Force -Scope CurrentUser
