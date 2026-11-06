Set-StrictMode -Version Latest
$script:MVCIGameHash='ccf02d07677f259165cc8830eba0fcfaeea32f70c756871735e7188c367be2e6'
$script:MVCIBin='MVCI\Binaries\Win64\'
$script:MVCIOwned=@('dinput8.dll','dinput8.ini','scripts\MVCIAccess.asi','scripts\MVCIAccess\prism.dll','scripts\MVCIAccess\README.md','scripts\MVCIAccess\licenses\MVCIAccess-GPL-3.0.txt','scripts\MVCIAccess\licenses\Prism-MPL-2.0.txt','scripts\MVCIAccess\licenses\MinHook.txt','scripts\MVCIAccess\licenses\PicoSHA2.txt','scripts\MVCIAccess\licenses\Ultimate-ASI-Loader.txt') | ForEach-Object {$script:MVCIBin+$_}
$script:MVCIOwned+=@(
 'manifest.json','coverage.json','opening-001.wav','opening-002.wav','opening-003.wav',
 'sanctuary-001.wav','sanctuary-002.wav','sanctuary-003.wav','xgard-001.wav','xgard-002.wav',
 'xgard-blasts.wav','xgard-slash.wav','d02-04.wav','d02-05.wav','d02-06.wav',
 'd03-robots.wav','d03-power.wav','d03-rings.wav','d03-bow.wav',
 'd05-01.wav','d05-02.wav','d05-03.wav','d05-05.wav','d05-06.wav','d05-07.wav',
 'd05-09.wav','d05-10.wav','d05-11.wav','d06-01.wav','d07-01.wav','d07-02.wav','d07-04.wav'
) | ForEach-Object {$script:MVCIBin+'scripts\MVCIAccess\descriptions\'+$_}
# Keep the 42 historical entries above for prior receipts. Each additional
# recording is owned by its exact path, independently of package manifests.
$script:MVCIOwned+=@(
 'story-0-a-01-001.wav','story-0-a-01-002.wav',
 'story-0-a-02-001.wav','story-0-a-02-002.wav','story-0-a-02-003.wav','story-0-a-02-004.wav',
 'story-0-a-03-001.wav','story-0-a-03-002.wav','story-0-a-03-003.wav','story-0-a-03-004.wav',
 'story-0-a-04-001.wav','story-0-a-04-002.wav','story-0-a-04-003.wav','story-0-a-04-004.wav',
 'story-0-a-04-005.wav','story-0-a-04-006.wav','story-0-a-04-007.wav',
 'story-0-a-05-001.wav','story-0-a-05-002.wav','story-0-a-05-003.wav','story-0-a-05-004.wav',
 'story-0-a-05-005.wav',
 'story-0-b-01-001.wav','story-0-b-01-002.wav',
 'story-0-b-02-001.wav','story-0-b-02-002.wav','story-0-b-02-003.wav',
 'story-0-b-03-001.wav',
 'story-0-b-04-001.wav','story-0-b-04-002.wav',
 'story-0-b-05-001.wav','story-0-b-05-002.wav',
 'story-0-b-06-001.wav','story-0-b-06-002.wav','story-0-b-06-003.wav','story-0-b-06-004.wav',
 'story-0-b-07-001.wav','story-0-b-07-002.wav','story-0-b-07-003.wav',
 'story-0-b-08-001.wav','story-0-b-08-002.wav','story-0-b-08-003.wav','story-0-b-08-004.wav',
 'story-1-b-01-001.wav','story-1-b-01-002.wav','story-1-b-01-003.wav','story-1-b-01-004.wav',
 'story-1-b-01-005.wav','story-1-b-01-006.wav',
 'story-1-b-02-001.wav','story-1-b-02-002.wav','story-1-b-02-003.wav',
 'story-1-b-03-001.wav','story-1-b-03-002.wav',
 'story-1-b-04-001.wav','story-1-b-04-002.wav','story-1-b-04-003.wav','story-1-b-04-004.wav',
 'story-1-b-04-005.wav','story-1-b-04-006.wav','story-1-b-04-007.wav','story-1-b-04-008.wav',
 'story-1-b-04-009.wav','story-1-b-04-010.wav','story-1-b-04-011.wav','story-1-b-04-012.wav',
 'story-1-b-04-013.wav','story-1-b-04-014.wav',
 'story-1-b-05-001.wav','story-1-b-05-002.wav','story-1-b-05-003.wav','story-1-b-05-004.wav',
 'story-1-b-05-005.wav','story-1-b-05-006.wav','story-1-b-05-007.wav',
 'story-1-c-01-001.wav','story-1-c-01-002.wav','story-1-c-01-003.wav','story-1-c-01-004.wav',
 'story-1-c-01-005.wav','story-1-c-01-006.wav','story-1-c-01-007.wav',
 'story-1-c-02-001.wav','story-1-c-02-002.wav','story-1-c-02-003.wav','story-1-c-02-004.wav',
 'story-1-c-02-005.wav','story-1-c-02-006.wav','story-1-c-02-007.wav','story-1-c-02-008.wav',
 'story-1-c-03-001.wav','story-1-c-03-002.wav','story-1-c-03-003.wav','story-1-c-03-004.wav',
 'story-2-d-01-001.wav','story-2-d-01-002.wav','story-2-d-01-003.wav',
 'story-2-d-02-001.wav','story-2-d-02-002.wav','story-2-d-02-003.wav','story-2-d-02-004.wav',
 'story-2-d-03-001.wav','story-2-d-03-002.wav',
 'story-2-d-04-001.wav','story-2-d-04-002.wav','story-2-d-04-003.wav',
 'story-3-a-01-001.wav','story-3-a-01-002.wav','story-3-a-01-003.wav','story-3-a-01-004.wav',
 'story-3-a-02-001.wav','story-3-a-02-002.wav',
 'story-3-a-03-001.wav','story-3-a-03-002.wav','story-3-a-03-003.wav','story-3-a-03-004.wav',
 'story-3-a-03-005.wav','story-3-a-03-006.wav',
 'story-3-a-04-001.wav','story-3-a-04-002.wav','story-3-a-04-003.wav','story-3-a-04-004.wav',
 'story-3-a-04-005.wav',
 'story-3-a-05-001.wav','story-3-a-05-002.wav','story-3-a-05-003.wav','story-3-a-05-004.wav',
 'story-3-a-05-005.wav','story-3-a-05-006.wav',
 'story-3-a-06-001.wav','story-3-a-06-002.wav','story-3-a-06-003.wav','story-3-a-06-004.wav',
 'story-3-a-06-005.wav','story-3-a-06-006.wav',
 'story-3-c-01-001.wav','story-3-c-01-002.wav','story-3-c-01-003.wav','story-3-c-01-004.wav',
 'story-3-c-02-001.wav','story-3-c-02-002.wav','story-3-c-02-003.wav','story-3-c-02-004.wav',
 'story-3-c-02-005.wav','story-3-c-02-006.wav','story-3-c-02-007.wav','story-3-c-02-008.wav',
 'story-3-c-03-001.wav','story-3-c-03-002.wav','story-3-c-03-003.wav','story-3-c-03-004.wav',
 'story-3-c-03-005.wav','story-3-c-03-006.wav','story-3-c-03-007.wav',
 'story-3-c-04-001.wav','story-3-c-04-002.wav','story-3-c-04-003.wav',
 'story-3-c-05-001.wav','story-3-c-05-002.wav','story-3-c-05-003.wav','story-3-c-05-004.wav',
 'story-3-c-05-005.wav','story-3-c-05-006.wav',
 'story-3-d-01-001.wav','story-3-d-01-002.wav','story-3-d-01-003.wav','story-3-d-01-004.wav',
 'story-3-d-01-005.wav',
 'story-3-d-02-001.wav','story-3-d-02-002.wav','story-3-d-02-003.wav','story-3-d-02-004.wav',
 'story-3-d-02-005.wav','story-3-d-02-006.wav','story-3-d-02-007.wav','story-3-d-02-008.wav',
 'story-3-d-03-001.wav','story-3-d-03-002.wav','story-3-d-03-003.wav','story-3-d-03-004.wav',
 'story-3-d-03-005.wav','story-3-d-03-006.wav',
 'story-3-e-01-001.wav','story-3-e-01-002.wav','story-3-e-01-003.wav','story-3-e-01-004.wav',
 'story-3-e-01-005.wav','story-3-e-01-006.wav','story-3-e-01-007.wav',
 'story-3-e-02-001.wav','story-3-e-02-002.wav','story-3-e-02-003.wav','story-3-e-02-004.wav',
 'story-3-e-02-005.wav',
 'story-3-e-03-001.wav','story-3-e-03-002.wav','story-3-e-03-003.wav','story-3-e-03-004.wav',
 'story-3-e-03-005.wav','story-3-e-03-006.wav','story-3-e-03-007.wav','story-3-e-03-008.wav',
 'story-3-e-03-009.wav','story-3-e-03-010.wav',
 'story-3-e-04-001.wav','story-3-e-04-002.wav','story-3-e-04-003.wav','story-3-e-04-004.wav',
 'story-3-e-04-005.wav','story-3-e-04-006.wav','story-3-e-04-007.wav','story-3-e-04-008.wav',
 'story-3-f-01-001.wav','story-3-f-01-002.wav','story-3-f-01-003.wav','story-3-f-01-004.wav',
 'story-3-f-01-005.wav','story-3-f-01-006.wav',
 'story-3-f-02-001.wav','story-3-f-02-002.wav',
 'story-3-f-03-001.wav','story-3-f-03-002.wav',
 'story-3-f-04-001.wav','story-3-f-04-002.wav',
 'story-3-f-05-001.wav','story-3-f-05-002.wav','story-3-f-05-003.wav','story-3-f-05-004.wav',
 'story-3-f-05-005.wav','story-3-f-05-006.wav','story-3-f-05-007.wav','story-3-f-05-008.wav',
 'story-3-f-05-009.wav',
 'story-3-f-06-001.wav','story-3-f-06-002.wav','story-3-f-06-003.wav','story-3-f-06-004.wav',
 'story-3-f-06-005.wav','story-3-f-06-006.wav','story-3-f-06-007.wav','story-3-f-06-008.wav',
 'story-3-f-06-009.wav','story-3-f-06-010.wav','story-3-f-06-011.wav',
 'story-3-f-07-001.wav','story-3-f-07-002.wav','story-3-f-07-003.wav','story-3-f-07-004.wav',
 'story-3-f-07-005.wav',
 'story-3-g-01-001.wav','story-3-g-01-002.wav','story-3-g-01-003.wav','story-3-g-01-004.wav',
 'story-3-g-01-005.wav',
 'story-3-g-02-001.wav','story-3-g-02-002.wav','story-3-g-02-003.wav','story-3-g-02-004.wav',
 'story-3-g-02-005.wav','story-3-g-02-006.wav',
 'story-3-g-03-001.wav','story-3-g-03-002.wav',
 'story-3-g-04-001.wav','story-3-g-04-002.wav','story-3-g-04-003.wav','story-3-g-04-004.wav',
 'story-3-g-05-001.wav','story-3-g-05-002.wav','story-3-g-05-003.wav','story-3-g-05-004.wav',
 'story-3-g-05-005.wav','story-3-g-05-006.wav','story-3-g-05-007.wav',
 'story-3-g-06-001.wav','story-3-g-06-002.wav','story-3-g-06-003.wav','story-3-g-06-004.wav',
 'story-3-g-06-005.wav','story-3-g-06-006.wav',
 'story-3-g-07-001.wav','story-3-g-07-002.wav','story-3-g-07-003.wav','story-3-g-07-004.wav',
 'story-3-g-08-001.wav','story-3-g-08-002.wav','story-3-g-08-003.wav','story-3-g-08-004.wav',
 'story-3-g-08-005.wav','story-3-g-08-006.wav','story-3-g-08-007.wav','story-3-g-08-008.wav',
 'story-3-g-08-009.wav',
 'story-3-g-09-001.wav','story-3-g-09-002.wav','story-3-g-09-003.wav','story-3-g-09-004.wav',
 'story-3-g-09-005.wav',
 'story-3-g-10-001.wav','story-3-g-10-002.wav','story-3-g-10-003.wav',
 'story-3-g-11-001.wav','story-3-g-11-002.wav','story-3-g-11-003.wav','story-3-g-11-004.wav',
 'story-3-g-11-005.wav','story-3-g-11-006.wav','story-3-g-11-007.wav','story-3-g-11-008.wav',
 'story-3-g-11-009.wav','story-3-g-11-010.wav','story-3-g-11-011.wav','story-3-g-11-012.wav',
 'story-3-g-11-013.wav','story-3-g-11-014.wav','story-3-g-11-015.wav','story-3-g-11-016.wav',
 'story-3-g-11-017.wav','story-3-g-11-018.wav','story-3-g-11-019.wav','story-3-g-11-020.wav',
 'story-4-a-01-001.wav','story-4-a-01-002.wav','story-4-a-01-003.wav',
 'story-4-a-02-001.wav','story-4-a-02-002.wav','story-4-a-02-003.wav','story-4-a-02-004.wav',
 'story-4-a-03-001.wav','story-4-a-03-002.wav','story-4-a-03-003.wav',
 'story-4-b-01-001.wav','story-4-b-01-002.wav','story-4-b-01-003.wav','story-4-b-01-004.wav',
 'story-4-b-02-001.wav','story-4-b-02-002.wav','story-4-b-02-003.wav','story-4-b-02-004.wav',
 'story-4-b-02-005.wav','story-4-b-02-006.wav',
 'story-4-b-03-001.wav','story-4-b-03-002.wav','story-4-b-03-003.wav','story-4-b-03-004.wav',
 'story-4-b-03-005.wav',
 'story-4-c-01-001.wav','story-4-c-01-002.wav','story-4-c-01-003.wav','story-4-c-01-004.wav',
 'story-4-c-01-005.wav',
 'story-4-c-02-001.wav','story-4-c-02-002.wav','story-4-c-02-003.wav','story-4-c-02-004.wav',
 'story-4-g-01-001.wav','story-4-g-01-002.wav','story-4-g-01-003.wav','story-4-g-01-004.wav',
 'story-4-g-01-005.wav',
 'story-4-g-02-001.wav','story-4-g-02-002.wav','story-4-g-02-003.wav','story-4-g-02-004.wav',
 'story-4-g-02-005.wav','story-4-g-02-006.wav','story-4-g-02-007.wav',
 'story-4-g-03-001.wav','story-4-g-03-002.wav','story-4-g-03-003.wav','story-4-g-03-004.wav',
 'story-4-g-03-005.wav','story-4-g-03-006.wav','story-4-g-03-007.wav','story-4-g-03-008.wav',
 'story-4-g-03-009.wav','story-4-g-03-010.wav','story-4-g-03-011.wav','story-4-g-03-012.wav',
 'story-4-g-03-013.wav','story-4-g-03-014.wav','story-4-g-03-015.wav',
 'story-5-a-01-001.wav','story-5-a-01-002.wav','story-5-a-01-003.wav','story-5-a-01-004.wav',
 'story-5-a-02-001.wav','story-5-a-02-002.wav','story-5-a-02-003.wav',
 'story-5-a-03-001.wav','story-5-a-03-002.wav',
 'story-5-b-01-001.wav','story-5-b-01-002.wav','story-5-b-01-003.wav','story-5-b-01-004.wav',
 'story-5-b-01-005.wav','story-5-b-01-006.wav','story-5-b-01-007.wav','story-5-b-01-008.wav',
 'story-5-b-01-009.wav','story-5-b-01-010.wav','story-5-b-01-011.wav','story-5-b-01-012.wav',
 'story-5-b-01-013.wav',
 'story-5-b-02-001.wav','story-5-b-02-002.wav','story-5-b-02-003.wav','story-5-b-02-004.wav',
 'story-5-b-02-005.wav','story-5-b-02-006.wav','story-5-b-02-007.wav','story-5-b-02-008.wav',
 'story-5-b-03-001.wav','story-5-b-03-002.wav','story-5-b-03-003.wav','story-5-b-03-004.wav',
 'story-5-b-03-005.wav','story-5-b-03-006.wav','story-5-b-03-007.wav','story-5-b-03-008.wav',
 'story-5-b-03-009.wav','story-5-b-03-010.wav','story-5-b-03-011.wav','story-5-b-03-012.wav',
 'story-5-b-03-013.wav','story-5-b-03-014.wav','story-5-b-03-015.wav','story-5-b-03-016.wav',
 'story-5-b-03-017.wav','story-5-b-03-018.wav',
 'story-5-b-04-001.wav','story-5-b-04-002.wav','story-5-b-04-003.wav','story-5-b-04-004.wav',
 'story-5-b-04-005.wav',
 'story-5-c-01-001.wav','story-5-c-01-002.wav','story-5-c-01-003.wav','story-5-c-01-004.wav',
 'story-5-c-01-005.wav'
) | ForEach-Object {$script:MVCIBin+'scripts\MVCIAccess\descriptions\'+$_}
# An explicit Steam root supports portable installations and isolated installer checks.
function Find-MVCIGamePath([string]$SteamRoot = $env:MVCIACCESS_STEAM_ROOT) {
 $gameExe = $script:MVCIBin + 'MVCI.exe'
 if ([string]::IsNullOrWhiteSpace($SteamRoot)) {
  $uninstallRoots = @(
   'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall'
   'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
   'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall'
  )
  $registeredGames = foreach ($root in $uninstallRoots) {
   $key = Join-Path $root 'Steam App 493840'
   $folder = Get-ItemPropertyValue -LiteralPath $key -Name InstallLocation -ErrorAction SilentlyContinue
   if ($folder -and (Test-Path -LiteralPath $folder -PathType Container) -and
       (Test-Path -LiteralPath (Join-Path $folder $gameExe) -PathType Leaf)) {
    (Resolve-Path -LiteralPath $folder).ProviderPath
   }
  }
  $registeredGames = @($registeredGames | Sort-Object -Unique)
  if ($registeredGames.Count -gt 1) { throw 'Steam registers more than one MVC Infinite game folder.' }
  if ($registeredGames.Count -eq 1) { return $registeredGames[0] }

  $SteamRoot = Get-ItemPropertyValue -LiteralPath 'HKCU:\Software\Valve\Steam' -Name SteamPath -ErrorAction SilentlyContinue
  if (!$SteamRoot) {
   $SteamRoot = Get-ItemPropertyValue -LiteralPath 'HKLM:\Software\WOW6432Node\Valve\Steam' -Name InstallPath -ErrorAction SilentlyContinue
  }
  if (!$SteamRoot) { $SteamRoot = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Steam' }
 }

 if (!(Test-Path -LiteralPath $SteamRoot -PathType Container)) { throw 'The Steam folder could not be found.' }
 $libraries = @($SteamRoot)
 $libraryFile = Join-Path $SteamRoot 'steamapps\libraryfolders.vdf'
 $libraryText = Get-Content -LiteralPath $libraryFile -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
 if ($libraryText) {
  # Current Steam uses path entries. Older files store the library path under its number.
  foreach ($entry in [regex]::Matches($libraryText, '(?im)^\s*"(?:path|\d+)"[ \t]+"([^"\r\n]+)"')) {
   $path = $entry.Groups[1].Value.Replace('\\', '\')
   if ($path -match '^(?:[A-Za-z]:[\\/]|\\\\)') { $libraries += $path }
  }
 }

 $installedGames = foreach ($library in ($libraries | Sort-Object -Unique)) {
  if (!(Test-Path -LiteralPath $library -PathType Container)) { continue }
  $steamApps = Join-Path $library 'steamapps'
  $appFile = Join-Path $steamApps 'appmanifest_493840.acf'
  $appText = Get-Content -LiteralPath $appFile -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
  if (!$appText -or $appText -notmatch '(?im)^\s*"appid"[ \t]+"493840"') { continue }
  $installName = [regex]::Match($appText, '(?im)^\s*"installdir"[ \t]+"([^"\r\n]+)"').Groups[1].Value
  if (!$installName) { continue }
  if ($installName.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0 -or $installName -in @('.', '..')) { continue }
  $folder = Join-Path (Join-Path $steamApps 'common') $installName
  if (Test-Path -LiteralPath (Join-Path $folder $gameExe) -PathType Leaf) {
   (Resolve-Path -LiteralPath $folder).ProviderPath
  }
 }
 $installedGames = @($installedGames | Sort-Object -Unique)
 if ($installedGames.Count -gt 1) { throw 'Steam lists more than one MVC Infinite game folder.' }
 if ($installedGames.Count -eq 0) { throw 'MVC Infinite was not found in the Steam libraries.' }
 return $installedGames[0]
}

function Install-MVCIAccess {
 param([string]$GameRoot,[string]$PackageRoot,[string]$ExpectedGameHash=$script:MVCIGameHash)
 if(Get-Process -Name MVCI -ErrorAction SilentlyContinue){throw 'Close MVC Infinite normally before installing.'}
 $taskGame=(Resolve-Path -LiteralPath $GameRoot).Path
 $taskPackage=(Resolve-Path -LiteralPath $PackageRoot).Path
 $taskExe=Get-MVCISafePath $taskGame ($script:MVCIBin+'MVCI.exe')
 if((Get-MVCIHash $taskExe) -ne $ExpectedGameHash -or !(Test-MVCIx64 $taskExe)){throw 'Unsupported MVC Infinite executable. Nothing installed.'}
 $taskManifest=Get-Content -LiteralPath (Join-Path $taskPackage 'manifest.json') -Raw|ConvertFrom-Json
 Test-MVCIManifest $taskGame $taskManifest
 if($taskManifest.game_sha256 -ne $ExpectedGameHash){throw 'Package targets another game build.'}
 $taskPayload=Join-Path $taskPackage 'payload'
 $taskDeclared=@{};foreach($taskFile in $taskManifest.files){
  $taskSource=Get-MVCISafePath $taskPayload $taskFile.path
  if(!(Test-Path -LiteralPath $taskSource -PathType Leaf) -or (Get-Item -LiteralPath $taskSource).Length -ne $taskFile.size -or (Get-MVCIHash $taskSource) -ne $taskFile.sha256){throw "Package verification failed: $($taskFile.path)"}
  if($taskFile.path -match '\.(asi|dll)$' -and !(Test-MVCIx64 $taskSource)){throw "Non-x64 payload: $($taskFile.path)"}
  $taskDeclared[$taskFile.path]=$true
 }
 $taskPayloadFiles=@(Get-ChildItem -LiteralPath $taskPayload -File -Recurse -Force)
 if($taskDeclared.Count -ne $script:MVCIOwned.Count){throw 'Incomplete MVCIAccess package.'}
 if($taskPayloadFiles.Count -ne $taskDeclared.Count){throw 'Undeclared or missing package files.'}
 foreach($taskPayloadFile in $taskPayloadFiles){$taskRel=$taskPayloadFile.FullName.Substring($taskPayload.Length+1);if(!$taskDeclared.ContainsKey($taskRel)){throw 'Undeclared package file.'}}
 $taskReceiptPath=Get-MVCISafePath $taskGame 'MVCIAccess.install.json'
 $taskOld=$null;$taskOldFiles=@{}
 if(Test-Path -LiteralPath $taskReceiptPath){$taskOld=Get-Content -LiteralPath $taskReceiptPath -Raw|ConvertFrom-Json;Test-MVCIManifest $taskGame $taskOld;foreach($taskFile in $taskOld.files){$taskOldFiles[$taskFile.path]=$taskFile}}
 $taskPlugin=Get-MVCIOwnedPath $taskGame ($script:MVCIBin+'scripts\MVCIAccess.asi')
 $taskOther=@(Get-ChildItem -LiteralPath (Join-Path $taskGame $script:MVCIBin) -Filter '*.asi' -File -Recurse -ErrorAction SilentlyContinue|Where-Object {$_.FullName -ne $taskPlugin})
 $taskSharedPaths=@(($script:MVCIBin+'dinput8.dll'),($script:MVCIBin+'dinput8.ini'))
 foreach($taskFile in $taskManifest.files){
  $taskDest=Get-MVCIOwnedPath $taskGame $taskFile.path
  if(Test-Path -LiteralPath $taskDest){
   if(!(Test-Path -LiteralPath $taskDest -PathType Leaf) -or !$taskOldFiles.ContainsKey($taskFile.path)){throw "Preserving unowned collision: $taskDest"}
   if((Get-MVCIHash $taskDest) -ne $taskOldFiles[$taskFile.path].sha256){throw "Preserving changed installed file: $taskDest"}
   if($taskOther.Count -gt 0 -and $taskFile.path -in $taskSharedPaths -and (Get-MVCIHash $taskDest) -ne $taskFile.sha256){throw "Preserving shared loader: $taskDest"}
  }
 }
 $taskBackup=Get-MVCISafePath $taskGame ('MVCIAccess-Backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N'))
 $taskChanged=@();$taskReceiptBacked=$false;$taskPriorReceipt=if(Test-Path -LiteralPath $taskReceiptPath){[IO.File]::ReadAllBytes($taskReceiptPath)}else{$null}
 try{
  foreach($taskFile in $taskManifest.files){
   $taskDest=Get-MVCIOwnedPath $taskGame $taskFile.path
   if((Test-Path -LiteralPath $taskDest) -and (Get-MVCIHash $taskDest) -eq $taskFile.sha256){continue}
   if($null -ne $taskPriorReceipt -and !$taskReceiptBacked){$null=New-Item -ItemType Directory -Path $taskBackup -Force;[IO.File]::WriteAllBytes((Get-MVCISafePath $taskBackup 'previous-receipt.json'),$taskPriorReceipt);$taskReceiptBacked=$true}
   $taskExisted=Test-Path -LiteralPath $taskDest
   if($taskExisted){$taskOriginal=Get-MVCISafePath $taskBackup $taskFile.path;$null=New-Item -ItemType Directory -Path (Split-Path -Parent $taskOriginal) -Force;Copy-Item -LiteralPath $taskDest -Destination $taskOriginal}
   $null=New-Item -ItemType Directory -Path (Split-Path -Parent $taskDest) -Force
   $taskBytes=[IO.File]::ReadAllBytes((Get-MVCISafePath $taskPayload $taskFile.path));Write-MVCIAtomic $taskDest $taskBytes
   $taskChanged+=@{path=$taskFile.path;existed=$taskExisted}
   if((Get-MVCIHash $taskDest) -ne $taskFile.sha256){throw 'Installed hash verification failed.'}
  }
  if((Get-MVCIHash $taskExe) -ne $ExpectedGameHash){throw 'Original executable changed during installation.'}
  $taskJson=$taskManifest|ConvertTo-Json -Depth 8
  Write-MVCIAtomic $taskReceiptPath ([Text.Encoding]::UTF8.GetBytes($taskJson))
 }catch{
  $taskFailure=$_
  for($taskIndex=$taskChanged.Count-1;$taskIndex -ge 0;$taskIndex--){$taskChange=$taskChanged[$taskIndex];$taskDest=Get-MVCIOwnedPath $taskGame $taskChange.path;if($taskChange.existed){Write-MVCIAtomic $taskDest ([IO.File]::ReadAllBytes((Get-MVCISafePath $taskBackup $taskChange.path)))}elseif(Test-Path -LiteralPath $taskDest){Remove-Item -LiteralPath $taskDest}}
  if($null -ne $taskPriorReceipt){Write-MVCIAtomic $taskReceiptPath $taskPriorReceipt}elseif(Test-Path -LiteralPath $taskReceiptPath){Remove-Item -LiteralPath $taskReceiptPath}
  throw $taskFailure
 }
 [pscustomobject]@{product='MVCIAccess';version=$taskManifest.version;files=$taskManifest.files.Count;changed=$taskChanged.Count;backup=if(Test-Path -LiteralPath $taskBackup){$taskBackup}else{$null}}
}
function Get-MVCISafePath([string]$Root,[string]$Relative){
 if([string]::IsNullOrEmpty($Relative) -or [IO.Path]::IsPathRooted($Relative) -or $Relative.Contains('/') -or $Relative.Contains(':')){throw 'Noncanonical deployment path.'}
 foreach($taskPart in $Relative.Split('\')){if($taskPart -in @('','.','..') -or $taskPart -match '[<>"|?*\x00-\x1f]' -or $taskPart -ne $taskPart.TrimEnd(' ','.') -or $taskPart -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)'){throw 'Unsafe deployment path.'}}
 $taskCanonicalRoot=[IO.Path]::GetFullPath($Root).TrimEnd('\');$taskAbsolute=[IO.Path]::GetFullPath((Join-Path $taskCanonicalRoot $Relative))
 if(!$taskAbsolute.StartsWith($taskCanonicalRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Deployment escaped root.'}
 $taskCurrent=$taskCanonicalRoot
 foreach($taskPart in @('')+$Relative.Split('\')){if($taskPart){$taskCurrent=Join-Path $taskCurrent $taskPart};$taskNode=Get-Item -LiteralPath $taskCurrent -Force -ErrorAction SilentlyContinue;if($null -ne $taskNode -and ($taskNode.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Preserving reparse-point path: $taskCurrent"}}
 return $taskAbsolute
}
function Get-MVCIOwnedPath([string]$Root,[string]$Relative){if($Relative -notin $script:MVCIOwned){throw 'Unexpected owned-file location.'};Get-MVCISafePath $Root $Relative}
function Get-MVCIHash([string]$Path){$taskStream=[IO.File]::OpenRead($Path);$taskHash=[Security.Cryptography.SHA256]::Create();try{[BitConverter]::ToString($taskHash.ComputeHash($taskStream)).Replace('-','').ToLowerInvariant()}finally{$taskStream.Dispose();$taskHash.Dispose()}}
function Test-MVCIManifest([string]$Root,$Manifest){
 if($Manifest.format -ne 1 -or $Manifest.product -ne 'MVCIAccess' -or !$Manifest.version -or $Manifest.game_sha256 -notmatch '^[a-fA-F0-9]{64}$' -or !$Manifest.files){throw 'Unrecognized MVCIAccess manifest.'}
 $taskSeen=@{};foreach($taskRecord in $Manifest.files){$null=Get-MVCIOwnedPath $Root $taskRecord.path;if($taskSeen.ContainsKey($taskRecord.path)){throw 'Duplicate owned-file record.'};$taskSeen[$taskRecord.path]=$true;if($taskRecord.sha256 -notmatch '^[a-fA-F0-9]{64}$' -or $taskRecord.size -lt 0 -or $taskRecord.size -gt 33554432){throw 'Invalid owned-file record.'}}
}
function Test-MVCIx64([string]$Path){
 $taskStream=[IO.File]::OpenRead($Path)
 try{$taskReader=New-Object IO.BinaryReader($taskStream);if($taskStream.Length -lt 64 -or $taskReader.ReadUInt16() -ne 0x5a4d){return $false};$taskStream.Position=0x3c;$taskOffset=$taskReader.ReadUInt32();if($taskOffset -gt $taskStream.Length-6){return $false};$taskStream.Position=$taskOffset;return ($taskReader.ReadUInt32() -eq 0x4550 -and $taskReader.ReadUInt16() -eq 0x8664)}finally{$taskStream.Dispose()}
}
function Write-MVCIAtomic([string]$Destination,[byte[]]$Bytes){
 $taskTemporary=$Destination+'.'+[guid]::NewGuid().ToString('N')+'.tmp'
 try{[IO.File]::WriteAllBytes($taskTemporary,$Bytes);if([IO.File]::Exists($Destination)){[IO.File]::Replace($taskTemporary,$Destination,[NullString]::Value)}else{[IO.File]::Move($taskTemporary,$Destination)}}finally{if(Test-Path -LiteralPath $taskTemporary){Remove-Item -LiteralPath $taskTemporary}}
}
function Uninstall-MVCIAccess([string]$GameRoot){
 if(Get-Process -Name MVCI -ErrorAction SilentlyContinue){throw 'Close MVC Infinite normally before uninstalling.'}
 $taskGame=(Resolve-Path -LiteralPath $GameRoot).Path;$taskReceiptPath=Get-MVCISafePath $taskGame 'MVCIAccess.install.json'
 $taskManifest=Get-Content -LiteralPath $taskReceiptPath -Raw|ConvertFrom-Json;Test-MVCIManifest $taskGame $taskManifest
 $taskRetained=@();$taskRemoved=0
 $taskPlugin=Get-MVCIOwnedPath $taskGame ($script:MVCIBin+'scripts\MVCIAccess.asi')
 $taskOther=@(Get-ChildItem -LiteralPath (Join-Path $taskGame $script:MVCIBin) -Filter '*.asi' -File -Recurse -ErrorAction SilentlyContinue|Where-Object {$_.FullName -ne $taskPlugin})
 $taskSharedPaths=@(($script:MVCIBin+'dinput8.dll'),($script:MVCIBin+'dinput8.ini'))
 foreach($taskFile in $taskManifest.files){
  $taskDest=Get-MVCIOwnedPath $taskGame $taskFile.path
  if(!(Test-Path -LiteralPath $taskDest)){continue}
  $taskChanged=(Get-MVCIHash $taskDest) -ne $taskFile.sha256
  $taskShared=$taskOther.Count -gt 0 -and $taskFile.path -in $taskSharedPaths
  if($taskChanged -or $taskShared){$taskRetained+=$taskFile;continue}
  Remove-Item -LiteralPath $taskDest;$taskRemoved++
 }
 if($taskRetained.Count){$taskManifest.files=@($taskRetained);Write-MVCIAtomic $taskReceiptPath ([Text.Encoding]::UTF8.GetBytes(($taskManifest|ConvertTo-Json -Depth 8)))}else{Remove-Item -LiteralPath $taskReceiptPath}
 [pscustomobject]@{removed=$taskRemoved;preserved_changed_or_shared=$taskRetained.Count}
}
