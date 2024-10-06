param(
    [Parameter(Mandatory = $true)]
    [string] $GroupPath,
    [string] $OutDir
)

$PSNativeCommandUseErrorActionPreference = $true
$ErrorActionPreference = "Stop"

if (!(Test-Path $GroupPath)) {
    Write-Error "Group file not found: $GroupPath"
    exit 1
}

$groupName = [System.IO.Path]::GetFileName($GroupPath)

$parts = ($Env:LC_GROUPS | ConvertFrom-Json -AsHashtable)[$groupName]["parts"]
$tempDir = [System.IO.Directory]::CreateTempSubdirectory()

$updatePath = $groupName + ".c4u"

if ($OutDir) {
    $updatePath = Join-Path $OutDir $updatePath
}

$updatePath = [System.IO.Path]::GetFullPath($updatePath, $PWD)
$GroupPath = Resolve-Path -Path $GroupPath

Remove-Item $updatePath -ErrorAction SilentlyContinue

$c4group = Resolve-Path -Path $Env:C4GROUP

foreach ($part in $parts) {
    Write-Output "Downloading $part..."
    $uri = "https://github.com/legacyclonk/LegacyClonk/releases/download/$part/$groupName"
    Invoke-WebRequest -Uri $uri -OutFile $(Join-Path $tempDir.FullName $groupName)
    Write-Output "Adding support for $part..."

    Push-Location -Path $tempDir.FullName

    try {
        & $c4group "$updatePath" -g "$groupName" "$GroupPath" $Env:OBJVERSION
    }
    finally {
        Pop-Location
    }
}

