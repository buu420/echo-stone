$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '..\tools\Deployment.Common.ps1')
$taskRoot=Join-Path ([IO.Path]::GetTempPath()) ('MVCIAccess-tests-'+[guid]::NewGuid().ToString('N'))
$taskChecks=0
function Assert-Task([bool]$Condition,[string]$Message){$script:taskChecks++;if(!$Condition){throw $Message}}
function Expect-TaskFailure([scriptblock]$Action,[string]$Pattern=''){$taskFailed=$false;try{& $Action|Out-Null}catch{if($Pattern -and $_.Exception.Message -notmatch $Pattern){throw};$taskFailed=$true};Assert-Task $taskFailed 'Expected refusal did not occur.'}
function Write-TaskPE([string]$Path,[int]$Machine=0x8664){
 $taskBytes=New-Object byte[] 256;$taskBytes[0]=0x4d;$taskBytes[1]=0x5a;[BitConverter]::GetBytes([int]0x80).CopyTo($taskBytes,0x3c);$taskBytes[0x80]=0x50;$taskBytes[0x81]=0x45;[BitConverter]::GetBytes([uint16]$Machine).CopyTo($taskBytes,0x84);$null=New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force;[IO.File]::WriteAllBytes($Path,$taskBytes)
}
function New-TaskPackage([string]$Name){
 $taskPkg=Join-Path $taskRoot $Name;$null=New-Item -ItemType Directory -Path $taskPkg
 $taskRecords=@();foreach($taskRel in $script:MVCIOwned){$taskFile=Join-Path (Join-Path $taskPkg 'payload') $taskRel;if($taskRel -match '\.(dll|asi)$'){Write-TaskPE $taskFile}else{$null=New-Item -ItemType Directory -Path (Split-Path -Parent $taskFile) -Force;[IO.File]::WriteAllText($taskFile,'fixture '+$taskRel)};$taskRecords+=@{path=$taskRel;size=(Get-Item -LiteralPath $taskFile).Length;sha256=(Get-MVCIHash $taskFile)}}
 @{format=1;product='MVCIAccess';version='fixture';game_sha256=$script:taskGameHash;files=$taskRecords}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Encoding UTF8
 return $taskPkg
}
try{
 $taskGame=Join-Path $taskRoot 'game';$taskExe=Join-Path $taskGame ($script:MVCIBin+'MVCI.exe');Write-TaskPE $taskExe;$script:taskGameHash=(Get-MVCIHash $taskExe)
 $taskSave=Join-Path $taskGame 'MVCI\Saved\SaveGames\precious.sav';$null=New-Item -ItemType Directory -Path (Split-Path -Parent $taskSave) -Force;[IO.File]::WriteAllText($taskSave,'save sentinel')
 $taskPkg=New-TaskPackage 'package'
 # Candidate10's receipt lists only the original10 payloads. Upgrade must
 # accept that receipt while installing the exact new narration payload set.
 $taskLegacyGame=Join-Path $taskRoot 'legacy-upgrade';Write-TaskPE (Join-Path $taskLegacyGame ($script:MVCIBin+'MVCI.exe'))
 $taskCurrentOwned=$script:MVCIOwned
 try{$script:MVCIOwned=@($taskCurrentOwned|Select-Object -First 10);$taskLegacyPackage=New-TaskPackage 'legacy-package';Install-MVCIAccess -GameRoot $taskLegacyGame -PackageRoot $taskLegacyPackage -ExpectedGameHash $script:taskGameHash|Out-Null}finally{$script:MVCIOwned=$taskCurrentOwned}
 $taskUpgrade=Install-MVCIAccess -GameRoot $taskLegacyGame -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash
 Assert-Task ($taskUpgrade.files -eq $script:MVCIOwned.Count) 'Legacy upgrade did not install all current payloads.'
 Assert-Task (Test-Path -LiteralPath (Join-Path $taskLegacyGame ($script:MVCIBin+'scripts\MVCIAccess\descriptions\opening-001.wav'))) 'Legacy upgrade omitted narration.'
 Uninstall-MVCIAccess -GameRoot $taskLegacyGame|Out-Null
 # Candidate 12's 42-file receipt must survive the full Story expansion.
 $taskDescPrefix=$script:MVCIBin+'scripts\MVCIAccess\descriptions\'
 $taskCompletionClips=@('story-0-b-04-001.wav','story-5-c-01-001.wav')|ForEach-Object {$taskDescPrefix+$_}
 foreach($taskClip in $taskCompletionClips){Assert-Task ($taskClip -in $taskCurrentOwned) 'Completed Story recording is not explicitly owned.'}
 $taskCompletionGame=Join-Path $taskRoot 'candidate12-upgrade'
 Write-TaskPE (Join-Path $taskCompletionGame ($script:MVCIBin+'MVCI.exe'))
 try {
  $script:MVCIOwned=@($taskCurrentOwned|Select-Object -First 42)
  $taskCandidate12=New-TaskPackage 'candidate12-package'
  Install-MVCIAccess -GameRoot $taskCompletionGame -PackageRoot $taskCandidate12 -ExpectedGameHash $script:taskGameHash|Out-Null
 } finally {$script:MVCIOwned=$taskCurrentOwned}
 $taskPreviousCompletionReceipt=Get-MVCIHash (Join-Path $taskCompletionGame 'MVCIAccess.install.json')
 $taskPriorManifestPath=$taskDescPrefix+'manifest.json'
 $taskPreviousDescriptionManifest=Get-MVCIHash (Join-Path $taskCompletionGame $taskPriorManifestPath)
 $taskRetainedRecording=$taskDescPrefix+'d07-04.wav'
 $taskPreviousRecording=Get-MVCIHash (Join-Path $taskCompletionGame $taskRetainedRecording)
 $taskUnownedRecording=Join-Path $taskCompletionGame ($taskDescPrefix+'user-note.wav')
 [IO.File]::WriteAllText($taskUnownedRecording,'unrelated file')
 $taskNextManifestFile=Join-Path (Join-Path $taskPkg 'payload') $taskPriorManifestPath
 [IO.File]::AppendAllText($taskNextManifestFile,' completed catalogue')
 $taskNextPackageManifest=Join-Path $taskPkg 'manifest.json'
 $taskNextManifest=Get-Content -LiteralPath $taskNextPackageManifest -Raw|ConvertFrom-Json
 foreach($taskRecord in $taskNextManifest.files){if($taskRecord.path -eq $taskPriorManifestPath){$taskRecord.sha256=Get-MVCIHash $taskNextManifestFile;$taskRecord.size=(Get-Item -LiteralPath $taskNextManifestFile).Length}}
 $taskNextManifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $taskNextPackageManifest -Encoding UTF8
 $taskCompletion=Install-MVCIAccess -GameRoot $taskCompletionGame -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash
 Assert-Task ($taskCompletion.files -eq $taskCurrentOwned.Count -and $taskCompletion.changed -eq $taskCurrentOwned.Count-42+1) 'Candidate 12 upgrade omitted a new recording or touched an unchanged file.'
 foreach($taskClip in $taskCompletionClips){Assert-Task (Test-Path -LiteralPath (Join-Path $taskCompletionGame $taskClip)) 'Completed recording missing after upgrade.'}
 Assert-Task ((Get-MVCIHash (Join-Path $taskCompletionGame $taskRetainedRecording)) -eq $taskPreviousRecording) 'Candidate 12 recording changed.'
 Assert-Task ((Get-MVCIHash (Join-Path $taskCompletion.backup $taskPriorManifestPath)) -eq $taskPreviousDescriptionManifest) 'Previous description manifest was not backed up.'
 Assert-Task ((Get-MVCIHash (Join-Path $taskCompletion.backup 'previous-receipt.json')) -eq $taskPreviousCompletionReceipt) 'Previous 42-file receipt was not backed up.'
 $taskCompletionRemove=Uninstall-MVCIAccess -GameRoot $taskCompletionGame
 Assert-Task ($taskCompletionRemove.removed -eq $taskCurrentOwned.Count) 'Completed uninstall left unchanged owned recordings behind.'
 Assert-Task ([IO.File]::ReadAllText($taskUnownedRecording) -eq 'unrelated file') 'Completed uninstall modified an unrelated file.'
 # Use a fresh fixture below, where exactly one recording is replaced.
 $taskPkg=New-TaskPackage 'package-after-completion'
 # Candidate11 owns the original20 paths. Expansion adds recordings and
 # replaces mastered clips, preserving the previous receipt and changed WAV.
 $taskPriorStoryGame=Join-Path $taskRoot 'prior-story-upgrade';Write-TaskPE (Join-Path $taskPriorStoryGame ($script:MVCIBin+'MVCI.exe'))
 $taskExpandedClip=$script:MVCIBin+'scripts\MVCIAccess\descriptions\d02-04.wav'
 Assert-Task ($taskExpandedClip -in $script:MVCIOwned) 'Expanded Story recording is not explicitly owned.'
 try{$script:MVCIOwned=@($taskCurrentOwned|Select-Object -First 20);$taskPriorStoryPackage=New-TaskPackage 'prior-story-package';Install-MVCIAccess -GameRoot $taskPriorStoryGame -PackageRoot $taskPriorStoryPackage -ExpectedGameHash $script:taskGameHash|Out-Null}finally{$script:MVCIOwned=$taskCurrentOwned}
 $taskReplacedClip=$script:MVCIBin+'scripts\MVCIAccess\descriptions\xgard-001.wav'
 $taskPriorClipHash=Get-MVCIHash (Join-Path $taskPriorStoryGame $taskReplacedClip)
 $taskPriorStoryReceiptHash=Get-MVCIHash (Join-Path $taskPriorStoryGame 'MVCIAccess.install.json')
 $taskNewClip=Join-Path (Join-Path $taskPkg 'payload') $taskReplacedClip
 [IO.File]::AppendAllText($taskNewClip,' mastered replacement')
 $taskExpandedManifest=Get-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Raw|ConvertFrom-Json
 foreach($taskRecord in $taskExpandedManifest.files){if($taskRecord.path -eq $taskReplacedClip){$taskRecord.sha256=Get-MVCIHash $taskNewClip;$taskRecord.size=(Get-Item -LiteralPath $taskNewClip).Length}}
 $taskExpandedManifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Encoding UTF8
 $taskStoryUpgrade=Install-MVCIAccess -GameRoot $taskPriorStoryGame -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash
 Assert-Task ($taskStoryUpgrade.files -eq $script:MVCIOwned.Count -and $taskStoryUpgrade.changed -eq $script:MVCIOwned.Count-20+1) 'Candidate11 upgrade omitted expanded or replaced recordings.'
 Assert-Task (Test-Path -LiteralPath (Join-Path $taskPriorStoryGame $taskExpandedClip)) 'Expanded recording missing after upgrade.'
 Assert-Task ((Get-MVCIHash (Join-Path $taskStoryUpgrade.backup $taskReplacedClip)) -eq $taskPriorClipHash) 'Previous recording not preserved in upgrade backup.'
 Assert-Task ((Get-MVCIHash (Join-Path $taskStoryUpgrade.backup 'previous-receipt.json')) -eq $taskPriorStoryReceiptHash) 'Previous Candidate11 receipt not preserved.'
 $taskStoryRemove=Uninstall-MVCIAccess -GameRoot $taskPriorStoryGame
 Assert-Task ($taskStoryRemove.removed -eq $script:MVCIOwned.Count) 'Expanded uninstall left unchanged owned recordings behind.'
 Install-MVCIAccess -GameRoot $taskGame -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash|Out-Null
 Assert-Task (Test-Path -LiteralPath (Join-Path $taskGame ($script:MVCIBin+'scripts\MVCIAccess.asi'))) 'Plugin missing.'
 Assert-Task (Test-Path -LiteralPath (Join-Path $taskGame ($script:MVCIBin+'scripts\MVCIAccess\descriptions\opening-001.wav'))) 'Story voice clip missing.'
 Assert-Task ((Get-MVCIHash $taskExe) -eq $script:taskGameHash) 'Original executable changed.'
 Assert-Task ([IO.File]::ReadAllText($taskSave) -eq 'save sentinel') 'Save changed.'
 # A managed install masks the standalone receipt until Amethyst restores it.
 $taskOverlayReceipt=Join-Path $taskGame 'MVCIAccess.install.json'
 $taskStandaloneReceipt=[IO.File]::ReadAllBytes($taskOverlayReceipt)
 try {
  [IO.File]::WriteAllText($taskOverlayReceipt,'{"format":0,"product":"MVCIAccess","managed_by":"Amethyst"}')
  $taskMarkerHash=Get-MVCIHash $taskOverlayReceipt
  $taskManagedPlugin=Join-Path $taskGame ($script:MVCIBin+'scripts\MVCIAccess.asi')
  $taskManagedPluginHash=Get-MVCIHash $taskManagedPlugin
  Expect-TaskFailure {
   Install-MVCIAccess -GameRoot $taskGame -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash
  } 'managed by Amethyst'
  Expect-TaskFailure { Uninstall-MVCIAccess -GameRoot $taskGame } 'managed by Amethyst'
  Assert-Task ((Get-MVCIHash $taskOverlayReceipt) -eq $taskMarkerHash) 'Standalone command changed the Amethyst receipt.'
  Assert-Task ((Get-MVCIHash $taskManagedPlugin) -eq $taskManagedPluginHash) 'Standalone command changed the Amethyst plugin.'
 } finally {
  [IO.File]::WriteAllBytes($taskOverlayReceipt,$taskStandaloneReceipt)
 }
 # Identical reinstall is safe only with a verified ownership receipt.
 Install-MVCIAccess -GameRoot $taskGame -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash|Out-Null
 $taskPlugin=Join-Path $taskGame ($script:MVCIBin+'scripts\MVCIAccess.asi');[IO.File]::AppendAllText($taskPlugin,'user change')
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash} 'changed installed'
 Uninstall-MVCIAccess -GameRoot $taskGame|Out-Null
 Assert-Task ([IO.File]::ReadAllBytes($taskPlugin).Length -gt 256) 'Changed plugin was removed.'
 Assert-Task (Test-Path -LiteralPath $taskSave) 'Save removed.'
 # Fresh install refuses an existing unowned loader, even if byte-identical.
 $taskGame2=Join-Path $taskRoot 'collision';Write-TaskPE (Join-Path $taskGame2 ($script:MVCIBin+'MVCI.exe'));Write-TaskPE (Join-Path $taskGame2 ($script:MVCIBin+'dinput8.dll'))
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame2 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash}
 Assert-Task (!(Test-Path -LiteralPath (Join-Path $taskGame2 ($script:MVCIBin+'scripts\MVCIAccess.asi')))) 'Partial collision install.'
 # Each malformed package uses a clean destination, so a collision cannot mask it.
 $taskGame3=Join-Path $taskRoot 'negative';Write-TaskPE (Join-Path $taskGame3 ($script:MVCIBin+'MVCI.exe'))
 [IO.File]::AppendAllText((Join-Path $taskPkg ('payload\'+$script:MVCIBin+'dinput8.ini')),'tampered')
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash} 'Package verification'
 $taskPkg=New-TaskPackage 'path-package';$taskManifest=Get-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Raw|ConvertFrom-Json;$taskManifest.files[0].path='..\escape';$taskManifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Encoding UTF8
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash} 'owned-file location'
 $taskPkg=New-TaskPackage 'duplicate-package';$taskManifest=Get-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Raw|ConvertFrom-Json;$taskManifest.files[1].path=$taskManifest.files[0].path;$taskManifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Encoding UTF8
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash} 'Duplicate'
 $taskPkg=New-TaskPackage 'x86-package';$taskX86=Join-Path $taskPkg ('payload\'+$script:MVCIBin+'scripts\MVCIAccess.asi');Write-TaskPE $taskX86 0x14c;$taskManifest=Get-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Raw|ConvertFrom-Json;foreach($taskRecord in $taskManifest.files){if($taskRecord.path -like '*.asi'){$taskRecord.sha256=Get-MVCIHash $taskX86}};$taskManifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Encoding UTF8
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash} 'Non-x64'
 $taskPkg=New-TaskPackage 'valid-package'
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash ('0'*64)} 'Unsupported'
 [IO.File]::WriteAllText((Join-Path $taskPkg 'payload\undeclared.txt'),'extra')
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash} 'Undeclared'
 Remove-Item -LiteralPath (Join-Path $taskPkg 'payload\undeclared.txt')
 # Fault injection at the second atomic write proves restoration after a partial update.
 Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash|Out-Null
 $taskReceipt=Get-MVCIHash (Join-Path $taskGame3 'MVCIAccess.install.json');$taskLoader=Get-MVCIHash (Join-Path $taskGame3 ($script:MVCIBin+'dinput8.dll'))
 $taskManifest=Get-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Raw|ConvertFrom-Json
 foreach($taskRecord in $taskManifest.files){if($taskRecord.path -match 'dinput8\.(dll|ini)$'){$taskUpdate=Join-Path (Join-Path $taskPkg 'payload') $taskRecord.path;[IO.File]::AppendAllText($taskUpdate,'v2');$taskRecord.size=(Get-Item -LiteralPath $taskUpdate).Length;$taskRecord.sha256=Get-MVCIHash $taskUpdate}}
 $taskManifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $taskPkg 'manifest.json') -Encoding UTF8
 $script:taskOriginalAtomic=(Get-Command Write-MVCIAtomic).ScriptBlock;$script:taskWrites=0
 Set-Item function:Write-MVCIAtomic -Value {param([string]$Destination,[byte[]]$Bytes);$script:taskWrites++;if($script:taskWrites -eq 2){throw 'Injected atomic-write failure'};& $script:taskOriginalAtomic $Destination $Bytes}
 try{Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash} 'Injected'}finally{Set-Item function:Write-MVCIAtomic -Value $script:taskOriginalAtomic}
 Assert-Task ((Get-MVCIHash (Join-Path $taskGame3 ($script:MVCIBin+'dinput8.dll'))) -eq $taskLoader) 'Rollback did not restore the loader.'
 Assert-Task ((Get-MVCIHash (Join-Path $taskGame3 'MVCIAccess.install.json')) -eq $taskReceipt) 'Rollback changed the receipt.'
 Write-TaskPE (Join-Path $taskGame3 ($script:MVCIBin+'scripts\OtherMod.asi'))
 Expect-TaskFailure {Install-MVCIAccess -GameRoot $taskGame3 -PackageRoot $taskPkg -ExpectedGameHash $script:taskGameHash} 'shared loader'
 Assert-Task ((Get-MVCIHash (Join-Path $taskGame3 ($script:MVCIBin+'dinput8.dll'))) -eq $taskLoader) 'Update replaced a shared loader.'
 Uninstall-MVCIAccess -GameRoot $taskGame3|Out-Null
 Assert-Task (Test-Path -LiteralPath (Join-Path $taskGame3 ($script:MVCIBin+'dinput8.dll'))) 'Shared ASI loader was removed.'
 Assert-Task (Test-Path -LiteralPath (Join-Path $taskGame3 ($script:MVCIBin+'dinput8.ini'))) 'Shared ASI loader settings were removed.'
 Assert-Task (Test-Path -LiteralPath (Join-Path $taskGame3 ($script:MVCIBin+'scripts\OtherMod.asi'))) 'Another mod was removed.'
 Write-Output "$taskChecks deployment checks passed."
}finally{
 $taskResolved=[IO.Path]::GetFullPath($taskRoot);$taskTemp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath());if(!$taskResolved.StartsWith($taskTemp,[StringComparison]::OrdinalIgnoreCase) -or !(Split-Path -Leaf $taskResolved).StartsWith('MVCIAccess-tests-')){throw 'Unsafe fixture cleanup target.'}
 if(Test-Path -LiteralPath $taskResolved){Remove-Item -LiteralPath $taskResolved -Recurse -Force}
}
