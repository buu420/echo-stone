[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameFolder,
    [Parameter(Mandatory = $true)][string]$ModFolder
)

$ErrorActionPreference = 'Stop'
$bin = 'MVCI\Binaries\Win64'
$exe = Join-Path $GameFolder "$bin\MVCI.exe"
$gameHash = 'ccf02d07677f259165cc8830eba0fcfaeea32f70c756871735e7188c367be2e6'

function Get-EchoStoneHash([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
    } finally {
        $sha.Dispose()
        $stream.Dispose()
    }
}

if (Get-Process -Name MVCI -ErrorAction SilentlyContinue) {
    throw 'Close Marvel vs. Capcom: Infinite normally before installing Echo Stone.'
}
if (!(Test-Path -LiteralPath $exe -PathType Leaf) -or
    (Get-EchoStoneHash $exe) -ne $gameHash) {
    throw 'Echo Stone supports Steam build 18789511. This executable is unsupported.'
}
if ((Test-Path -LiteralPath (Join-Path $GameFolder 'MVCIAccess.install.json')) -or
    (Test-Path -LiteralPath (Join-Path $GameFolder "$bin\scripts\MVCIAccess.asi"))) {
    throw 'An existing standalone install or receipt was found. Use its uninstaller before switching to Amethyst. If it preserves files, continue using the Windows ZIP until the ownership conflict is resolved.'
}
foreach ($name in 'dinput8.dll', 'dinput8.ini') {
    $installed = Join-Path $GameFolder "$bin\$name"
    if (Test-Path -LiteralPath $installed) {
        $bundled = Join-Path $ModFolder "files\$bin\$name"
        if ((Get-EchoStoneHash $installed) -ne
            (Get-EchoStoneHash $bundled)) {
            throw "Conflicting $name in the Win64 folder. Echo Stone cannot use it."
        }
    }
}
Write-Output 'Echo Stone compatibility check passed.'
