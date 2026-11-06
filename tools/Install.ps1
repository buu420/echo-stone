[CmdletBinding()]
param([string]$GamePath='', [string]$PackagePath='')
$ErrorActionPreference='Stop'
if([string]::IsNullOrWhiteSpace($PackagePath)){$PackagePath=Split-Path -Parent $PSScriptRoot}
. (Join-Path $PSScriptRoot 'Deployment.Common.ps1')
if (!$PSBoundParameters.ContainsKey('GamePath')) { $GamePath = Find-MVCIGamePath }
if ([string]::IsNullOrWhiteSpace($GamePath)) { throw 'Provide a valid MVC Infinite game folder.' }
Install-MVCIAccess -GameRoot $GamePath -PackageRoot $PackagePath
