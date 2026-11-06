[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$GamePath)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Deployment.Common.ps1')
Uninstall-MVCIAccess -GameRoot $GamePath
