param([Parameter(Mandatory=$true)][string]$SourceRoot,[Parameter(Mandatory=$true)][string]$BuildDir)
$ErrorActionPreference='Stop'
$taskTemp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath());$taskRoot=Join-Path $taskTemp ('MVCIAccess-loader-test-'+[guid]::NewGuid().ToString('N'))
try{
 $null=New-Item -ItemType Directory -Path (Join-Path $taskRoot 'scripts') -Force
 Copy-Item -LiteralPath (Join-Path $BuildDir 'loader_host.exe') -Destination (Join-Path $taskRoot 'loader_host.exe')
 Copy-Item -LiteralPath (Join-Path $BuildDir 'MVCIContractMarker.asi') -Destination (Join-Path $taskRoot 'scripts\MVCIContractMarker.asi')
 Copy-Item -LiteralPath (Join-Path $SourceRoot 'dependencies\ual-x64\dinput8.dll') -Destination (Join-Path $taskRoot 'dinput8.dll')
 Copy-Item -LiteralPath (Join-Path $SourceRoot 'config\dinput8.ini') -Destination (Join-Path $taskRoot 'dinput8.ini')
 & (Join-Path $taskRoot 'loader_host.exe') (Join-Path $taskRoot 'dinput8.dll');if($LASTEXITCODE -ne 0){throw 'Pinned loader configuration did not initialize the ASI worker.'}
}finally{
 $taskResolved=[IO.Path]::GetFullPath($taskRoot)
 if(!$taskResolved.StartsWith($taskTemp,[StringComparison]::OrdinalIgnoreCase) -or !(Split-Path -Leaf $taskResolved).StartsWith('MVCIAccess-loader-test-')){throw 'Unsafe loader-fixture cleanup target.'}
 if(Test-Path -LiteralPath $taskResolved){Remove-Item -LiteralPath $taskResolved -Recurse -Force}
}
