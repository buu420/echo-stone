[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameFolder,
    [Parameter(Mandatory = $true)][string]$ModFolder
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Deployment.Common.ps1')
$bin = 'MVCI\Binaries\Win64'
$exe = Join-Path $GameFolder "$bin\MVCI.exe"

# Match the package builder's fingerprint across JSON formatting and property order.
function Get-EchoStoneReceiptFingerprint($Receipt) {
    $records = [string[]]@(foreach ($file in $Receipt.files) {
        $size = ([long]$file.size).ToString([Globalization.CultureInfo]::InvariantCulture)
        '{0}|{1}|{2}' -f $file.path, $size, $file.sha256
    })
    [Array]::Sort($records, [StringComparer]::Ordinal)
    $header = '{0}|{1}|{2}|{3}' -f $Receipt.format, $Receipt.product, $Receipt.version, $Receipt.game_sha256
    $bytes = [Text.Encoding]::UTF8.GetBytes($header + [char]10 + ($records -join [char]10))
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        [BitConverter]::ToString($sha.ComputeHash($bytes)).Replace('-', '').ToLowerInvariant()
    } finally {
        $sha.Dispose()
    }
}

if (Get-Process -Name MVCI -ErrorAction SilentlyContinue) {
    throw 'Close Marvel vs. Capcom: Infinite normally before installing Echo Stone.'
}
if (!(Test-Path -LiteralPath $exe -PathType Leaf) -or
    (Get-MVCIHash $exe) -ne $script:MVCIGameHash) {
    throw 'Echo Stone supports Steam build 18789511. This executable is unsupported.'
}
$receiptPath = Get-MVCISafePath $GameFolder 'MVCIAccess.install.json'
if (Test-Path -LiteralPath $receiptPath) {
    if (!(Test-Path -LiteralPath $receiptPath -PathType Leaf) -or
        (Get-Item -LiteralPath $receiptPath).Length -gt 1MB) {
        throw 'The standalone Echo Stone receipt is unreadable.'
    }
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    Test-MVCIManifest $GameFolder $receipt
    $knownPath = Join-Path $PSScriptRoot 'EchoStone-standalone-receipts.json'
    $known = Get-Content -LiteralPath $knownPath -Raw | ConvertFrom-Json
    $expected = $known.PSObject.Properties[[string]$receipt.version]
    if ($null -eq $expected -or $receipt.game_sha256 -ne $script:MVCIGameHash -or
        $receipt.files.Count -ne $script:MVCIOwned.Count -or
        $expected.Value -ne (Get-EchoStoneReceiptFingerprint $receipt)) {
        throw 'The standalone receipt does not match a supported Echo Stone Windows ZIP.'
    }
    foreach ($file in $receipt.files) {
        $path = Get-MVCIOwnedPath $GameFolder $file.path
        if (!(Test-Path -LiteralPath $path -PathType Leaf) -or
            (Get-Item -LiteralPath $path).Length -ne $file.size -or
            (Get-MVCIHash $path) -ne $file.sha256) {
            throw "The standalone Echo Stone file differs: $($file.path). Restore the original Windows ZIP files before switching."
        }
    }
    Write-Output "Verified standalone Echo Stone $($receipt.version). Amethyst uninstall will restore this copy."
} elseif (Test-Path -LiteralPath (Join-Path $GameFolder "$bin\scripts\MVCIAccess.asi")) {
    throw 'Echo Stone files are present without a standalone receipt. Switch from an intact Windows ZIP installation.'
}
foreach ($name in 'dinput8.dll', 'dinput8.ini') {
    $installed = Join-Path $GameFolder "$bin\$name"
    if (Test-Path -LiteralPath $installed) {
        $bundled = Join-Path $ModFolder "files\$bin\$name"
        if ((Get-MVCIHash $installed) -ne
            (Get-MVCIHash $bundled)) {
            throw "Conflicting $name in the Win64 folder. Echo Stone cannot use it."
        }
    }
}
Write-Output 'Echo Stone compatibility check passed.'
