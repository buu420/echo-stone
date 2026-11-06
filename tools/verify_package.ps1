[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$Archive,
 [string]$OriginalExe='',
 [string]$Evidence='',
 [switch]$AutomaticInstall
)
$ErrorActionPreference='Stop'
if([string]::IsNullOrWhiteSpace($Evidence)){$Evidence=Join-Path (Split-Path -Parent $PSScriptRoot) 'artifacts\package-install-check.json'}
. (Join-Path $PSScriptRoot 'Deployment.Common.ps1')
if (!$OriginalExe) { $OriginalExe = Join-Path (Find-MVCIGamePath) ($script:MVCIBin+'MVCI.exe') }
$taskTemp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$taskFixture=Join-Path $taskTemp ('MVCIAccess-package-check-'+[guid]::NewGuid().ToString('N'))
$taskSteam=Join-Path $taskFixture 'Steam'
$taskLibrary=Join-Path $taskFixture ('Library with spaces '+[char]0x03a9)
$taskGameName='Game with spaces '+[char]0x03a9
$taskGame=Join-Path $taskLibrary ('steamapps\common\'+$taskGameName)
try{
 $taskExe=Join-Path $taskGame ($script:MVCIBin+'MVCI.exe')
 $null=New-Item -ItemType Directory -Path (Split-Path -Parent $taskExe) -Force
 Copy-Item -LiteralPath $OriginalExe -Destination $taskExe
 if((Get-MVCIHash $taskExe) -ne $script:MVCIGameHash){throw 'The isolated fixture needs the exact supported original.'}
 $taskSave=Join-Path $taskGame 'MVCI\Saved\SaveGames\fixture.sav'
 $null=New-Item -ItemType Directory -Path (Split-Path -Parent $taskSave) -Force
 [IO.File]::WriteAllText($taskSave,'Isolated fixture sentinel; never a real save.')
 $taskSaveHash=Get-MVCIHash $taskSave
 Add-Type -AssemblyName System.IO.Compression.FileSystem
 $taskUnzip=Join-Path $taskFixture 'extracted'
 [IO.Compression.ZipFile]::ExtractToDirectory([IO.Path]::GetFullPath($Archive),$taskUnzip)
 $taskPackage=Join-Path $taskUnzip ([IO.Path]::GetFileNameWithoutExtension($Archive))
 $taskInstaller = @('Install-EchoStone.cmd', 'Install-MVCIAccess.cmd') |
  Where-Object { Test-Path -LiteralPath (Join-Path $taskPackage $_) -PathType Leaf } |
  Select-Object -First 1
 $taskUsedCommand = [bool]$taskInstaller
 if($AutomaticInstall -and !$taskUsedCommand){throw 'The automatic installer command is missing.'}
 if($taskUsedCommand){
  $taskSteamApps=Join-Path $taskSteam 'steamapps'
  $null=New-Item -ItemType Directory -Path $taskSteamApps -Force
  $taskLibraryValue=$taskLibrary.Replace('\','\\')
  [IO.File]::WriteAllText((Join-Path $taskSteamApps 'libraryfolders.vdf'), @"
"libraryfolders"
{
 "1"
 {
  "path" "$taskLibraryValue"
 }
}
"@)
  [IO.File]::WriteAllText((Join-Path $taskLibrary 'steamapps\appmanifest_493840.acf'), @"
"AppState"
{
 "appid" "493840"
 "installdir" "$taskGameName"
}
"@)
  $taskPriorSteamRoot=$env:MVCIACCESS_STEAM_ROOT
  $taskPriorEncoding=$OutputEncoding
  Push-Location -LiteralPath $taskPackage
  try {
   $env:MVCIACCESS_STEAM_ROOT=$taskSteam
   $OutputEncoding=[Console]::InputEncoding
   $taskInput=if($AutomaticInstall){''}else{@(('"{0}"' -f $taskGame), '')}
   $taskOutput=$taskInput | & $env:ComSpec /d /c $taskInstaller
   if($LASTEXITCODE -ne 0){throw "Installer command failed: $($taskOutput -join [Environment]::NewLine)"}
   if($AutomaticInstall -and ($taskOutput -join ' ') -match 'Press any key|Paste the full path'){
    throw 'The automatic installer requested input.'
   }
  } finally {
   $env:MVCIACCESS_STEAM_ROOT=$taskPriorSteamRoot
   $OutputEncoding=$taskPriorEncoding
   Pop-Location
  }
 }else{
  & (Join-Path $taskPackage 'tools\Install.ps1') -GamePath $taskGame | Out-Null
 }
 $taskManifest=Get-Content -LiteralPath (Join-Path $taskPackage 'manifest.json') -Raw|ConvertFrom-Json
 foreach($taskFile in $taskManifest.files){if((Get-MVCIHash (Get-MVCIOwnedPath $taskGame $taskFile.path)) -ne $taskFile.sha256){throw 'Extracted/install payload mismatch.'}}
 $taskRepeat=& (Join-Path $taskPackage 'tools\Install.ps1') -GamePath $taskGame -PackagePath $taskPackage
 if($taskRepeat.changed -ne 0){throw 'Identical repeat install changed payload files.'}
 $taskUninstall=& (Join-Path $taskPackage 'tools\Uninstall.ps1') -GamePath $taskGame
 if($taskUninstall.removed -ne $taskManifest.files.Count -or $taskUninstall.preserved_changed_or_shared -ne 0){throw 'Uninstall did not remove exactly the unchanged owned files.'}
 if((Get-MVCIHash $taskExe) -ne $script:MVCIGameHash -or (Get-MVCIHash $taskSave) -ne $taskSaveHash){throw 'Original fixture files changed.'}
 $taskResult=[ordered]@{passed=$true;archive=[IO.Path]::GetFullPath($Archive);archive_sha256=(Get-MVCIHash $Archive);owned_hashes_matched=$taskManifest.files.Count;repeat_files_changed=$taskRepeat.changed;uninstalled_files=$taskUninstall.removed;original_executable_preserved=$true;fixture_save_preserved=$true;unicode_and_spaces_path=$true;installer_command_tested=$taskUsedCommand;automatic_install_tested=[bool]$AutomaticInstall;executed_game=$false}
 $taskResult|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $Evidence -Encoding UTF8
 $taskResult|ConvertTo-Json -Depth 6
}finally{
 $taskResolved=[IO.Path]::GetFullPath($taskFixture)
 if(!$taskResolved.StartsWith($taskTemp,[StringComparison]::OrdinalIgnoreCase) -or !(Split-Path -Leaf $taskResolved).StartsWith('MVCIAccess-package-check-')){throw 'Unsafe package-fixture cleanup target.'}
 if(Test-Path -LiteralPath $taskResolved){Remove-Item -LiteralPath $taskResolved -Recurse -Force}
}
